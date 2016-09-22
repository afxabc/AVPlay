#pragma once

#include "base/buffer.h"
#include "base/queue.h"
#include "base/thread.h"

#include <mmsystem.h>
#include <dsound.h>
#pragma comment(lib, "dsound.lib")
#pragma comment(lib,"dxguid.lib")

using namespace eko;

class AudioPlay
{
public:
	AudioPlay();
	~AudioPlay();

public:
	bool start(UINT samplesPerSec, UINT channels, unsigned char bitsPerSample, int ptime = 40);
	void stop();
	void inputPcm(const char* data, int len);
	void reset();

	int setVolume(int vol);	//0-100
	int getVolume();

	bool isStart() { return started_; }
	void setMute(bool mute = true) { mute_ = mute; }

private:
	void loop();

private:
	static const int MAX_AUDIO_BUF = 4;

	WAVEFORMATEX fmtWave_;
	LPDIRECTSOUND8 lpDSound_;
	LPDIRECTSOUNDBUFFER8 pDSB8_;
	LPDIRECTSOUNDNOTIFY pDSN_;

	bool started_;
	bool mute_;

	Thread thread_;
	UINT bufferNotifySize_;
	Mutex mutex_;
	Buffer recvBuff_;
	Queue<Buffer> playQueue_;
	int ptime_;

	static const int VOL_RANG = 101;
	LONG volTable[VOL_RANG];
	int vol_;
};

