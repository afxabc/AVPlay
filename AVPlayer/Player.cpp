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

//////////////////////////////////////////////////////////////////////////

Player::Player()
	: pFormatCtx_(NULL)
	, pVCodecCtx_(NULL)
	, pVCodec_(NULL)
	, pFrameYUV_(NULL)
	, pFrameRGB_(NULL)
	, swsContext_(NULL)
	, q2d_(0.0)
	, paused_(true)
	, seeked_(true)
	, timeTotal_(0)
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
	for (int i = 0; i<pFormatCtx_->nb_streams; i++)
		if (pFormatCtx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex_ = i;
			break;
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
		LOGE("没有找到解码器: %X ！", pVCodecCtx_->codec_id);
		return false;
	}

	if (avcodec_open2(pVCodecCtx_, pVCodec_, NULL)<0)
	{
		LOGE("无法打开解码器：%X ！", pVCodecCtx_->codec_id);
		return false;
	}

	q2d_ = av_q2d(pFormatCtx_->streams[videoindex_]->time_base);
	timeTotal_ = pFormatCtx_->duration/1000;

	pFrameYUV_ = av_frame_alloc();
	pFrameRGB_ = av_frame_alloc();

	return (thDecode_.start(boost::bind(&Player::decodeLoop, this))
		&& thPlay_.start(boost::bind(&Player::playLoop, this))
		&& thSeek_.start(boost::bind(&Player::seekLoop, this))
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

	timeBase_ = timeDts_+TIMER;
	sigDecode_.on();
}

void Player::onTimer()
{
	if (paused_)
		return;

	Lock lock(mutex_);

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

	timeDts_ = dts * q2d_ * 1000;

	frmOut.width_ = pFrameYUV_->width;
	frmOut.height_ = pFrameYUV_->height;
	frmOut.size_ = frmOut.width_* frmOut.height_ * 4;
	frmOut.data_ = new BYTE[frmOut.size_];
	frmOut.tm_ = timeDts_;

	if (swsContext_ == NULL)
		swsContext_ = sws_getContext(frmOut.width_, frmOut.height_, pVCodecCtx_->pix_fmt, frmOut.width_, frmOut.height_, AV_PIX_FMT_BGRA, SWS_BICUBIC, NULL, NULL, NULL);

	avpicture_fill((AVPicture *)pFrameRGB_, frmOut.data_, AV_PIX_FMT_BGRA, frmOut.width_, frmOut.height_);
	sws_scale(swsContext_, pFrameYUV_->data, pFrameYUV_->linesize, 0, frmOut.height_, pFrameRGB_->data, pFrameRGB_->linesize);
	
	//queuePlay_.insert(frmOut, packet.pts * q2d * 1000);
	//直接播放可以，按pts播放反而错乱？？
	decodeFinish_(frmOut);

	return dts;
}

void Player::seekFrm()
{
	if (!seeked_)
		return;
	seeked_ = false;

	Lock lock(mutex_);
	double seekTm = (double)timeSeek_ / (1000 * q2d_);
	if (timeSeek_ > timeDts_)
	{
		av_seek_frame(pFormatCtx_, videoindex_, seekTm, 0);
		av_seek_frame(pFormatCtx_, videoindex_, seekTm - 1, AVSEEK_FLAG_BACKWARD);
	}
	else av_seek_frame(pFormatCtx_, videoindex_, seekTm - 1, AVSEEK_FLAG_BACKWARD);

	AVPacket packet;
	while (!seeked_)
	{
		if (av_read_frame(pFormatCtx_, &packet) < 0)
			break;

		int64_t dts = decodeVideo(packet);
		if (dts * q2d_ * 1000 >= timeSeek_)
		{
			createFrm(dts);
			break;
		}
	}
	timeBase_ = timeSeek_;
}

void Player::decodeLoop()
{
	AVPacket packet;

	timeSeek_ = 0;
	timeBase_ = 0;
	timeDts_ = 0;
	timePts_ = 0;
	MMRESULT mRes = ::timeSetEvent(TIMER, 0, &TimerProc, (DWORD)this, TIME_PERIODIC);

	sigDecode_.off();

	paused_ = false;
	seeked_ = false;
	while (thDecode_.started())
	{
		{
			Lock lock(mutex_);

			if (av_read_frame(pFormatCtx_, &packet) < 0)
			{
				paused_ = true;
				timeBase_ = timeDts_ - TIMER - TIMER;
			}
			else if (createFrm(decodeVideo(packet)) < 0)
				continue;
		}
	
		while (timeDts_ > timeBase_ && thDecode_.started())
			sigDecode_.wait();

	}

	av_packet_unref(&packet);

	while (queuePlay_.size() > 0)
		Thread::sleep(100);
	mRes = ::timeKillEvent(mRes);

	LOGW("+++++++++++++++ decodeLoop quit. +++++++++++++++");
}

void Player::seekLoop()
{
	sigSeek_.off();
	while (thSeek_.started())
	{
		sigSeek_.wait(500);
		seekFrm();
	}
	LOGW("################ seekLoop quit. ################");
}

void Player::playLoop()
{
	FrameData frm;
	int64_t when;

	sigPlay_.off();
	while (thPlay_.started())
	{
		while ((!queuePlay_.peerFront(timePts_) || timePts_ > timeBase_) && thPlay_.started())
			sigPlay_.wait(500);

		if (queuePlay_.getFront(frm))
		{

			LOGW("%d === %lf === %d", (int)timeBase_, frm.tm_, (int)timePts_);
			if (decodeFinish_)
				decodeFinish_(frm);
		}

	}
	queuePlay_.clear();
	LOGW("================ playLoop quit. ================");
}
