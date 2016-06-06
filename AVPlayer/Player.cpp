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
	, pCodecCtx_(NULL)
	, pCodec_(NULL)
	, paused_(true)
{
	av_register_all();
	pFormatCtx_ = avformat_alloc_context();
}


Player::~Player()
{
	stopPlay();
	if (pFormatCtx_)
		avformat_free_context(pFormatCtx_), pFormatCtx_ = NULL;
}

bool Player::startPlay(const char* finame)
{
	stopPlay();

	if (avformat_open_input(&pFormatCtx_, finame, NULL, NULL) != 0)
	{
		LOGE("无法打开文件：%s ！", finame);
		return false;
	}

	if (avformat_find_stream_info(pFormatCtx_, NULL)<0)
	{
		LOGE("无法获取流信息！");
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
		return false;
	}

	pCodecCtx_ = pFormatCtx_->streams[videoindex_]->codec;
	pFormatCtx_->video_codec_id = pCodecCtx_->codec_id;
	pFormatCtx_->video_codec = pCodec_ = avcodec_find_decoder(pCodecCtx_->codec_id);
	if (pCodec_ == NULL)
	{
		LOGE("没有找到解码器: %X ！", pCodecCtx_->codec_id);
		return false;
	}

	if (avcodec_open2(pCodecCtx_, pCodec_, NULL)<0)
	{
		LOGE("无法打开解码器：%X ！", pCodecCtx_->codec_id);
		return false;
	}

	return (thDecode_.start(boost::bind(&Player::decodeLoop, this))
			&& thPlay_.start(boost::bind(&Player::playLoop, this)));
}

void Player::stopPlay()
{
	sigDecode_.on();
	if (thDecode_.started())
		thDecode_.stop();

	sigPlay_.on();
	if (thPlay_.started())
		thPlay_.stop();

	if (pCodecCtx_)
		avcodec_close(pCodecCtx_),pCodecCtx_ = NULL;
	avformat_close_input(&pFormatCtx_);
}

static void CALLBACK TimerProc(UINT uiID, UINT uiMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	Player* pThis = (Player*)dwUser;
	if (pThis != NULL)
		pThis->onTimer();
}

void Player::onTimer()
{
	if (paused_)
		return;

	timeBase_ += TIMER;
	sigDecode_.on();
	sigPlay_.on();
}

void Player::decodeLoop()
{
	AVPacket packet;

	AVFrame *pFrame = av_frame_alloc();
	AVFrame *pFrameRGB = av_frame_alloc();
	SwsContext *sContext = NULL;

	double q2d = av_q2d(pFormatCtx_->streams[videoindex_]->time_base);

	paused_ = false;

	timeBase_ = 0;
	timeDts_ = 10000;
	timePts_ = 10000;
	MMRESULT mRes = ::timeSetEvent(TIMER, 0, &TimerProc, (DWORD)this, TIME_PERIODIC);

	sigDecode_.off();
	while (thDecode_.started())
	{
//		av_packet_unref(&packet);
		if (av_read_frame(pFormatCtx_, &packet) < 0)
			break;

		if (packet.data == NULL || videoindex_ != packet.stream_index)
		{
			av_packet_unref(&packet);
			continue;
		}

		int got_picture = 0;
		int ret = avcodec_decode_video2(pCodecCtx_, pFrame, &got_picture, &packet);
		if (ret <= 0)
		{
		//	LOGW("================== ret=%d +++++++++++++++", ret);
			av_packet_unref(&packet);
			continue;
		}

		if (got_picture > 0)
		{
			FrameData pout;
			pout.width_ = pFrame->width;
			pout.height_ = pFrame->height;
			pout.size_ = pout.width_* pout.height_ * 4;
			pout.data_ = new BYTE[pout.size_];
			pout.tm_ = packet.pts * q2d;

			if (sContext == NULL)
				sContext = sws_getContext(pout.width_, pout.height_, pCodecCtx_->pix_fmt, pout.width_, pout.height_, AV_PIX_FMT_BGRA, SWS_BICUBIC, NULL, NULL, NULL);

			avpicture_fill((AVPicture *)pFrameRGB, pout.data_, AV_PIX_FMT_BGRA, pout.width_, pout.height_);
			sws_scale(sContext, pFrame->data, pFrame->linesize, 0, pout.height_, pFrameRGB->data, pFrameRGB->linesize);

//			queuePlay_.insert(pout, packet.pts * q2d * 1000);
			//直接播放可以，按pts播放反而错乱？？
			decodeFinish_(pout);

			timeDts_ = packet.dts * q2d * 1000;
			while (timeDts_ > timeBase_ && thDecode_.started())
				sigDecode_.wait();

		}
		else
		{
			LOGW("*** got_picture=%d ***", got_picture);
		}
		av_packet_unref(&packet);

	}


	av_packet_unref(&packet);
	if (sContext != NULL)
		sws_freeContext(sContext);

	av_free(pFrameRGB);
	av_free(pFrame);

	while (queuePlay_.size() > 0)
		Thread::sleep(100);

	mRes = ::timeKillEvent(mRes);

	stopPlay();
	LOGW("+++++++++++++++ decodeLoop quit. +++++++++++++++");
}

void Player::playLoop()
{
	FrameData frm;
	int64_t when;

	sigPlay_.off();
	while (thPlay_.started())
	{
		while ((!queuePlay_.peerFront(timePts_) || timePts_ > timeBase_) && thPlay_.started())
			sigPlay_.wait();

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
