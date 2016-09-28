#pragma once

#include "FrameData.h"
#include "AudioPlay.h"

#include "timequeue.h"
#include "base\thread.h"
#include "base\queue.h"

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
#include "libswscale\swscale.h"
#include "libswresample\swresample.h"
};
#include <atomic>
#include "vld.h"

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
	void stopPlay(bool close_input = false);

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

	void tickForward();

	void seekReset();
	void seekTime(int64_t ms)
	{
		seeked_ = true;
		Lock lock(mutex_);
		seekQueue_.putBack((ms < 0) ? 0 : ms);
	}

	int64_t getTime()
	{
		return timeBase_;
	}

	int64_t getTimeTotal()
	{
		return timeTotal_;
	}

	void onTimer();

	int setVolume(int vol) { return aPlay_.setVolume(vol); }
	int getVolume() { return aPlay_.getVolume(); }

	void decodeLoop();
	void seekLoop();
	void playLoop();

private:
	void decodeAudio(AVPacket& packet);
	int64_t decodeVideo(AVPacket& packet);
	int64_t createFrm(int64_t dts);

	void closeInput();

	void seekFrm();

private:
	AVFormatContext *pFormatCtx_;

	int audioindex_;
	AVCodecContext  *pACodecCtx_;
	AVCodec         *pACodec_;
	AVFrame			*pFrameAudio_; 
	char			*pcmBuffer_;
	int				pcmBufferSize_;
	SwrContext		*swrContext_;
	AudioPlay  aPlay_;

	int videoindex_;
	AVCodecContext  *pVCodecCtx_;
	AVCodec         *pVCodec_;
	AVFrame			*pFrameYUV_;
	AVFrame			*pFrameRGB_;
	SwsContext		*swsContext_;
	std::atomic_int32_t vPending_;
	std::atomic_int32_t aPending_;
	double  q2d_;

	int64_t		timeTotal_;
	std::atomic_int64_t		timeBase_;
	int64_t		timeSeek_;
	int64_t		timeDtsV_;
	int64_t		timeDtsA_;
	int64_t		timePts_;

	static const int TIMER = 10;
	mutable Mutex mutex_;
	bool paused_;
	std::atomic_bool seeked_;
	std::atomic_bool ticked_;
	Queue<int64_t> seekQueue_;

	Thread thDecode_;
	Signal sigDecode_;

	Thread thSeek_;
	Signal sigSeek_;

	mutable Mutex mutexPts_;
	Thread thPlay_;
	Signal sigPlay_;
	TimeQueue<FrameData> queuePlayV_;
	TimeQueue<FrameData> queuePlayA_;

	FrameHandler decodeFinish_;

};



