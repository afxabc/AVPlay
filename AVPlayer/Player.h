#pragma once

#include "FrameData.h"
#include "timequeue.h"

#include "base\thread.h"
#include "base\queue.h"

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
#include "libswscale\swscale.h"
};

class AVPacketHold
{
public:
	AVPacketHold();
	AVPacketHold(AVPacket& packet);
	AVPacketHold(const AVPacketHold& p);

	~AVPacketHold();

	AVPacketHold& operator=(const AVPacketHold& rhs);

	void reset();

	operator AVPacket()
	{
		return packet_;
	}

	AVPacket& packet()
	{
		return packet_;
	}

private:
	void take(AVPacket & packet);

private:
	mutable AVPacket packet_;
};

//////////////////////////////////////////////////////////////

class Player
{
public:
	Player();
	~Player();

public:
	bool startPlay(const char* finame);
	void stopPlay();

	void setDecodeFinish(const FrameHandler& f)
	{
		decodeFinish_ = f;
	}

	bool isPlaying()
	{
		return (thDecode_.started());
	}

	bool isPaused()
	{
		return paused_;
	}

	void setPaused(bool paused)
	{
		paused_ = paused;
	}

	void seek(int ms)
	{
		timeSeek_ = timeBase_+ms;
	}

	void onTimer();

private:
	void decodeLoop();
	void playLoop();

private:
	AVFormatContext *pFormatCtx_;
	AVCodecContext  *pCodecCtx_;
	AVCodec         *pCodec_;
	int videoindex_;

	int64_t		timeBase_;
	int64_t		timeSeek_;
	int64_t		timeDts_;
	int64_t		timePts_;
	static const int TIMER = 10;
	mutable Mutex mutexPts_;
	bool paused_;

	Thread thDecode_;
	Signal sigDecode_;

	Thread thPlay_;
	Signal sigPlay_;
	TimeQueue<FrameData> queuePlay_;

	FrameHandler decodeFinish_;

};



