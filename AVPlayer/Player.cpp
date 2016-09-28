#include "stdafx.h"
#include "Player.h"

#include "base\log.h"

#include "mmsystem.h"

AVPacketHold::AVPacketHold()
{
	av_init_packet(&packet_);
	packet_.buf = NULL;
}

AVPacketHold::AVPacketHold(AVPacket & packet)
{
	av_init_packet(&packet_);
	packet_.buf = NULL;
	take(packet);
}

AVPacketHold::AVPacketHold(const AVPacketHold & p)
{
	av_init_packet(&packet_);
	packet_.buf = NULL;
	take(p.packet_);
}

AVPacketHold::~AVPacketHold()
{
	reset();
}

AVPacketHold & AVPacketHold::operator=(const AVPacketHold & rhs)
{
	take(rhs.packet_);
	return *this;
}

void AVPacketHold::reset()
{
	av_packet_unref(&packet_);
}

void AVPacketHold::take(AVPacket & packet)
{
	if (&packet == &packet_)
		return;

	reset();
	av_packet_ref(&packet_, &packet);
	av_packet_unref(&packet);
}


#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio 
//////////////////////////////////////////////////////////////////////////

Player::Player()
	: pFormatCtx_(NULL)
	, pACodecCtx_(NULL)
	, pACodec_(NULL)
	, swrContext_(NULL)
	, pFrameAudio_(NULL)
	, pcmBuffer_(NULL)
	, pVCodecCtx_(NULL)
	, pVCodec_(NULL)
	, pFrameYUV_(NULL)
	, pFrameRGB_(NULL)
	, swsContext_(NULL)
	, q2d_(0.0)
	, paused_(true)
	, seeked_(true)
	, ticked_(true)
	, timeTotal_(0)
	, vPending_(0)
	, aPending_(0)
{
	av_register_all();
	pFormatCtx_ = avformat_alloc_context();
}


Player::~Player()
{
	stopPlay(true);
	if (pFormatCtx_)
		avformat_free_context(pFormatCtx_), pFormatCtx_ = NULL;
}

bool Player::startPlay(const char* finame)
{
	stopPlay(true);

	if (avformat_open_input(&pFormatCtx_, finame, NULL, NULL) != 0)
	{
		LOGE("无法打开文件：%s ！", finame);
		return false;
	}

	if (avformat_find_stream_info(pFormatCtx_, NULL)<0)
	{
		LOGE("无法获取流信息！");
		closeInput();
		return false;
	}

	videoindex_ = -1;
	audioindex_ = -1;
	for (int i = 0; i < pFormatCtx_->nb_streams; i++)
	{
		if (pFormatCtx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && videoindex_ < 0)
			videoindex_ = i;
		else if (pFormatCtx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audioindex_ < 0)
			audioindex_ = i;
		
//		avcodec_close(pFormatCtx_->streams[i]->codec);


//		if (videoindex_ > 0 && audioindex_ > 0)
//			break;
	}

	if (videoindex_ == -1)
	{
		LOGE("无法获取视频流！");
		closeInput();
		return false;
	}

	pVCodecCtx_ = pFormatCtx_->streams[videoindex_]->codec;
	pFormatCtx_->video_codec_id = pVCodecCtx_->codec_id;
	pFormatCtx_->video_codec = pVCodec_ = avcodec_find_decoder(pVCodecCtx_->codec_id);
	if (pVCodec_ == NULL)
	{
		LOGE("没有找到视频解码器: %X ！", pVCodecCtx_->codec_id);
		return false;
	}

	if (avcodec_open2(pVCodecCtx_, pVCodec_, NULL)<0)
	{
		LOGE("无法打开视频解码器：%X ！", pVCodecCtx_->codec_id);
		return false;
	}

	q2d_ = av_q2d(pFormatCtx_->streams[videoindex_]->time_base);
	timeTotal_ = pFormatCtx_->duration/1000;

	pFrameYUV_ = av_frame_alloc();
	pFrameRGB_ = av_frame_alloc();

	if (audioindex_ >= 0)
	{
		pACodecCtx_ = pFormatCtx_->streams[audioindex_]->codec;
		pFormatCtx_->audio_codec_id = pACodecCtx_->codec_id;
		pFormatCtx_->audio_codec = pACodec_ = avcodec_find_decoder(pACodecCtx_->codec_id);
		if (pACodec_ != NULL)
		{
			if (avcodec_open2(pACodecCtx_, pACodec_, NULL) >= 0)
			{
				pFrameAudio_ = av_frame_alloc();
				pcmBuffer_ = new char[MAX_AUDIO_FRAME_SIZE*2];
			}
			else LOGW("无法打开音频解码器：%X ！", pACodecCtx_->codec_id);
		}
		else
		{
			LOGW("没有找到音频解码器: %X ！", pACodecCtx_->codec_id);
		}
	}

	return (thDecode_.start([this]() {this->decodeLoop(); })
		&& thPlay_.start([this]() {this->playLoop(); })
		&& thSeek_.start([this]() {this->seekLoop(); })
		);
}

void Player::closeInput()
{
	sigSeek_.on();
	if (thSeek_.started())
		thSeek_.stop();

	if (swsContext_)
		sws_freeContext(swsContext_), swsContext_ = NULL;

	if (pFrameRGB_)
		av_free(pFrameRGB_), pFrameRGB_ = NULL;

	if (pFrameYUV_)
		av_free(pFrameYUV_), pFrameYUV_ = NULL;

	if (pVCodecCtx_)
		avcodec_close(pVCodecCtx_), pVCodecCtx_ = NULL;

	if (pFrameAudio_)
		av_free(pFrameAudio_), pFrameAudio_ = NULL;

	if (pcmBuffer_)
		delete[] pcmBuffer_, pcmBuffer_ = NULL;

	if (pACodecCtx_)
		avcodec_close(pACodecCtx_), pACodecCtx_ = NULL;

	if (swrContext_)
		swr_free(&swrContext_), swrContext_ = NULL;

	aPlay_.stop();

	avformat_close_input(&pFormatCtx_);
}

void Player::stopPlay(bool close_input)
{
	paused_ = false;
	sigDecode_.on();
	if (thDecode_.started())
		thDecode_.stop();

	sigPlay_.on();
	if (thPlay_.started())
		thPlay_.stop();

	if (close_input)
		closeInput();

}

static void CALLBACK TimerProc(UINT uiID, UINT uiMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	Player* pThis = (Player*)dwUser;
	if (pThis != NULL)
		pThis->onTimer();
}

void Player::tickForward()
{
	paused_ = true;
	Lock lock(mutex_);

//	timeBase_ += TIMER+ TIMER;
//	timeBase_ = timeDtsV_ + TIMER;
	timeBase_ = timePts_ + TIMER;
	if (timeBase_ > timeTotal_)
		timeBase_ = timeTotal_;

	sigDecode_.on();
	sigPlay_.on();
}

void Player::seekReset()
{
	vPending_ = 0;
	aPending_ = 0;
	queuePlayV_.clear();
	queuePlayA_.clear();
	aPlay_.reset();
}

void Player::onTimer()
{
	if (paused_ || (seekQueue_.size() > 0))
		return;

	Lock lock(mutex_);

	if (timeBase_ < timeTotal_)
		timeBase_ += TIMER;
	sigDecode_.on();
	sigPlay_.on();
}

int64_t Player::decodeVideo(AVPacket & packet)
{
	int64_t dts = -1;
	if (packet.data == NULL || videoindex_ != packet.stream_index)
	{
		goto END;
	}

	int got_picture = 0;
	av_frame_unref(pFrameYUV_);
	int ret = avcodec_decode_video2(pVCodecCtx_, pFrameYUV_, &got_picture, &packet);
	if (ret <= 0)
	{
		//	LOGW("================== ret=%d +++++++++++++++", ret);
		goto END;
	}

	if (got_picture <= 0)
	{
		LOGW("*** got_picture=%d ***", got_picture);
		goto END;
	}

	dts = packet.dts;

END:
	av_packet_unref(&packet);
	return dts;
}

int64_t Player::createFrm(int64_t dts)
{
	if (dts < 0)
		return dts;

	FrameData frmOut;

	timeDtsV_ = dts * q2d_ * 1000;

	frmOut.type_ = FRAME_VIDEO;
	frmOut.width_ = pFrameYUV_->width;
	frmOut.height_ = pFrameYUV_->height;
	frmOut.size_ = frmOut.width_* frmOut.height_ * 4;
	frmOut.data_ = new BYTE[frmOut.size_];
	frmOut.tm_ = timeDtsV_;

	if (swsContext_ == NULL)
		swsContext_ = sws_getContext(frmOut.width_, frmOut.height_, pVCodecCtx_->pix_fmt, frmOut.width_, frmOut.height_, AV_PIX_FMT_BGRA, SWS_BICUBIC, NULL, NULL, NULL);

	avpicture_fill((AVPicture *)pFrameRGB_, frmOut.data_, AV_PIX_FMT_BGRA, frmOut.width_, frmOut.height_);
	sws_scale(swsContext_, pFrameYUV_->data, pFrameYUV_->linesize, 0, frmOut.height_, pFrameRGB_->data, pFrameRGB_->linesize);
	av_frame_unref(pFrameYUV_);
	
	//queuePlay_.insert(frmOut, packet.pts * q2d * 1000);
	vPending_++;
//	LOGW("################ %d", vPending_);
	queuePlayV_.insert(frmOut, timeDtsV_);
	//直接播放可以，按pts播放反而错乱？？
	//decodeFinish_(frmOut);

	return dts;
}

void Player::decodeAudio(AVPacket & packet)
{
	if (packet.data == NULL || audioindex_ != packet.stream_index)
	{
		av_packet_unref(&packet);
		return;
	}

	int got_picture = 0;
	int sz = 0;
	while ((sz = avcodec_decode_audio4(pACodecCtx_, pFrameAudio_, &got_picture, &packet)) > 0)
	{
		if (got_picture <= 0)
		{
			LOGW("*** got_picture=%d ***", got_picture);
			break;
		}

		/////////////////////////////////////////

		if (swrContext_ == NULL)
		{
			uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
			AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
			int out_nb_samples = 1024;
			int out_sample_rate = 44100;
			int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

			pcmBufferSize_ = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

			swrContext_ = swr_alloc();
			int64_t in_channel_layout = av_get_default_channel_layout(pACodecCtx_->channels);
			swrContext_ = swr_alloc_set_opts(swrContext_, out_channel_layout, out_sample_fmt, out_sample_rate,
				in_channel_layout, pACodecCtx_->sample_fmt, pACodecCtx_->sample_rate, 0, NULL);
			swr_init(swrContext_);

			aPlay_.start(out_sample_rate, out_channels, 16, pFrameAudio_->nb_samples*2000/ pFrameAudio_->sample_rate);
		}

		int nb = swr_convert(swrContext_, (unsigned char**)&pcmBuffer_, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)pFrameAudio_->data, pFrameAudio_->nb_samples);
		if (nb > 0)
		{
			FrameData frmOut;

			timeDtsA_ = packet.pts*av_q2d(pFormatCtx_->streams[audioindex_]->time_base) * 1000- 500;

			frmOut.type_ = FRAME_AUDIO;
			frmOut.tm_ = timeDtsA_;
			frmOut.size_ = nb * 2 * 2;
			frmOut.data_ = new BYTE[frmOut.size_];
			memcpy(frmOut.data_, pcmBuffer_, frmOut.size_);

			queuePlayA_.insert(frmOut, timeDtsA_);
			aPending_++;
		//	aPlay_.inputPcm(pcmBuffer_, nb*2*2);
		}
		else LOGW("=== swr_convert return %d, pFrameAudio_->nb_samples=%d", nb, pFrameAudio_->nb_samples);

		////////////////////////////////////////

		packet.data += sz;
		packet.size -= sz;
		if (packet.size <= 0)
			break;
	}

	av_packet_unref(&packet);
}

void Player::seekFrm()
{
	if (seekQueue_.size() <= 0)
		return;

	ticked_ = false;

	{
		Lock lock(mutex_);
		seekQueue_.getBack(timeSeek_);
		seekQueue_.clear();
	}

	if (timeSeek_ > timeTotal_ - TIMER)
		timeSeek_ = timeTotal_ - TIMER;

	double seekTm = (double)timeSeek_ / (1000 * q2d_);

	int seek_ret = 0;
	if (timeSeek_ > timeDtsV_)
	{
		seek_ret = av_seek_frame(pFormatCtx_, videoindex_, seekTm, 0);
		seek_ret = av_seek_frame(pFormatCtx_, videoindex_, seekTm - TIMER, AVSEEK_FLAG_BACKWARD);
	}
	else
		seek_ret = av_seek_frame(pFormatCtx_, videoindex_, seekTm - TIMER, AVSEEK_FLAG_BACKWARD);

	if (seek_ret < 0)
	{
		ticked_ = true;
		return;
	}


	AVPacket packet;
	av_init_packet(&packet);


	bool get1 = false;
	int64_t dts = 0;
	while (thDecode_.started())
	{
		if (seekQueue_.size() > 0)
		{
			LOGW("*** new seek income !!! =%d ***", seekQueue_.size());
			break;
		}

		if (av_read_frame(pFormatCtx_, &packet) < 0)
		{
			LOGW("*** seek last frame got=%d ***", get1);
			break;
		}

		if (videoindex_ != packet.stream_index)
		{
			av_packet_unref(&packet);
			continue;
		}

		dts = decodeVideo(packet);

		if (dts < 0)
		{
			LOGW("*** decodeVideo failed=%d !!! ***", dts);
			continue;
		}
		else
		{
			get1 = true;
		}

		if (dts * q2d_ * 1000 >= timeSeek_)
		{
			break;
		}
	}

	av_packet_unref(&packet);

	if (get1)
	{
		seekReset();
		timeBase_ = timeSeek_ = dts * q2d_ * 1000;
		createFrm(dts);
		sigPlay_.on();
	}

	ticked_ = true;
}

void Player::decodeLoop()
{
	AVPacket packet;
	av_init_packet(&packet);

	timeSeek_ = 0;
	timeBase_ = 0;
	timeDtsV_ = 0;
	timeDtsA_ = 0;
	timePts_ = 0;
	vPending_ = 0;
	aPending_ = 0;
	ticked_ = true;

	seekQueue_.clear();

	MMRESULT mRes = ::timeSetEvent(TIMER, 0, &TimerProc, (DWORD)this, TIME_PERIODIC);

	sigDecode_.off();

	paused_ = false;
	seeked_ = false;

	bool end = false;
	while (thDecode_.started())
	{
		if (seekQueue_.size() <= 0)
		{
			Lock lock(mutex_);

			end = false;
			if (av_read_frame(pFormatCtx_, &packet) < 0)
			{
			//	paused_ = true;
				end = true;
			//	timeBase_ = timeDtsV_;
			}
			else if (videoindex_ == packet.stream_index)
			{
				createFrm(decodeVideo(packet));
			}
			else if (audioindex_ == packet.stream_index)
			{
				decodeAudio(packet);
			}
				
			av_packet_unref(&packet);
		}
		else seekFrm();

		while (thDecode_.started())
		{
		//	int64_t tmMin = (timeDtsV_ > timeDtsA_)?timeDtsA_:timeDtsV_;
			int64_t tmMin = timeDtsV_;
		//	if (tmMin <= timeBase_+2000 && !end)
			if ((queuePlayV_.size() <= 60 && !end) || seekQueue_.size()>0)
				break;
			//LOGW("++++++ wait...v=%d, a=%d, B=%d , qV=%d, qA=%d++++++", (int)timeDtsV_, (int)timeDtsA_, (int)timeBase_,
			//	queuePlayV_.size(), queuePlayA_.size());
			sigDecode_.wait(100);
			end = false;
		}
	}

	av_packet_unref(&packet);

	mRes = ::timeKillEvent(mRes);

	LOGW("+++++++++++++++ decodeLoop quit. +++++++++++++++");
}

void Player::seekLoop()
{
	sigSeek_.off();
	while (thSeek_.started())
	{
		sigSeek_.wait(500);
	//	seekFrm();
	}
	LOGW("################ seekLoop quit. ################");
}

void Player::playLoop()
{
	FrameData frm;
	eko::MicroSecond when(0);
	int span = 0;

	bool gotFrm = false;

	sigPlay_.off();
	while (thPlay_.started())
	{
		gotFrm = false;

		timePts_ = 0;
		span = 0;
		if (queuePlayV_.peerFront(when))
		{
			timePts_ = when;
			if (when <= timeBase_ && queuePlayV_.getFront(frm))
			{
	//			LOGW("%d === %d === %d --- %d", (int)frm.type_, (int)timeBase_, (int)when, frm.size_);
				vPending_--;
				decodeFinish_(frm);
				gotFrm = true;
			}
			else span = (when - timeBase_);
		}

		if (queuePlayA_.peerFront(when))
		{
			if (when <= timeBase_ && queuePlayA_.getFront(frm))
			{
	//			LOGW("%d ===  ===  --- %d", (int)when, frm.size_);
				aPending_--;
				if (!aPlay_.inputPcm((char*)frm.data_, frm.size_))
					LOGW("%d === %d ---- qV=%d, qA=%d", (int)when, frm.size_, queuePlayV_.size(), queuePlayA_.size());
				gotFrm = true;
			}
			else span = ((when - timeBase_) > span)?span:(when - timeBase_);
		}
			
		if (!gotFrm)
		{
			sigPlay_.wait(span);

		}
	}
	queuePlayV_.clear();
	queuePlayA_.clear();
	LOGW("================ playLoop quit. ================");
}
