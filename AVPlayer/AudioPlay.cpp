#include "stdafx.h"
#include "AudioPlay.h"

AudioPlay::AudioPlay()
	: lpDSound_(NULL)
	, pDSB8_(NULL)
	, pDSN_(NULL)
	, started_(false)
	, mute_(false)
	, ptime_(40)
{
	HRESULT hr = DirectSoundCreate8(NULL, &lpDSound_, NULL);

	vol_ = VOL_RANG / 2;
	double k = (DSBVOLUME_MAX - DSBVOLUME_MIN) / log10((double)VOL_RANG);
	for (int i = 0; i < VOL_RANG; i++)
	{
		volTable[i] = k * log10((double)(i+1) / VOL_RANG);
	}
}


AudioPlay::~AudioPlay()
{
	stop();
	lpDSound_->Release();
}

bool AudioPlay::start(UINT samplesPerSec, UINT channels, unsigned char bitsPerSample, int ptime)
{
	stop();

	//疑问，必须>=40
	ptime_ = (ptime >= 40) ? ptime : 40;
	
	HRESULT hr = lpDSound_->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
	if (FAILED(hr))
	{
		TRACE("SetCooperativeLevel failed %X\n", hr);
		return false;
	}
	
	//{WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0}; 
	fmtWave_.wFormatTag = WAVE_FORMAT_PCM;
	fmtWave_.nChannels = channels;
	fmtWave_.nSamplesPerSec = samplesPerSec;
	fmtWave_.nAvgBytesPerSec = samplesPerSec*channels*bitsPerSample/8;
	fmtWave_.nBlockAlign = bitsPerSample/8* channels;
	fmtWave_.wBitsPerSample = bitsPerSample;
	fmtWave_.cbSize = sizeof(fmtWave_);

	bufferNotifySize_ = fmtWave_.nAvgBytesPerSec*ptime_ / 1000;

	//创建辅助缓冲区对象
	DSBUFFERDESC dsbd;
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags =
		DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLFX | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 |
		DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;
	dsbd.dwBufferBytes = bufferNotifySize_*MAX_AUDIO_BUF;	//((alaw_)?playSpan_*2:playSpan_); 
	dsbd.lpwfxFormat = &fmtWave_;

	pDSB8_ = NULL;
	pDSN_ = NULL;
	LPDIRECTSOUNDBUFFER pDSB = NULL;

	//获取接口
	bool res = false;
	do
	{
		if (FAILED(hr = lpDSound_->CreateSoundBuffer(&dsbd, &pDSB, NULL)))
		{
			TRACE("CreateSoundBuffer failed 0x%X \n", hr);
			hr = E_INVALIDARG;
			break;
		}
		if (FAILED(hr = pDSB->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&pDSB8_)))
		{
			TRACE("QueryInterface IID_IDirectSoundBuffer8 failed %X\n", hr);
			break;
		}
		if (FAILED(hr = pDSB->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&pDSN_)))
		{
			TRACE("QueryInterface IID_IDirectSoundNotify failed %X\n", hr);
			break;
		}
		res = true;
	} while (0);

	if (pDSB != NULL)
		pDSB->Release();

	if (!res || !thread_.start([this]() {this->loop(); }))
	{
		if (pDSB8_ != NULL)
			pDSB8_->Release();
		if (pDSN_ != NULL)
			pDSN_->Release();
		return false;
	}

	return true;
}

void AudioPlay::stop()
{
	if (!started_)
		return;

	started_ = false;
	thread_.stop();
	playQueue_.clear();
	recvBuff_.erase();
}

bool AudioPlay::inputPcm(const char * data, int len)
{
	if (!isStart() || mute_)
	{
		if (recvBuff_.readableBytes() > 0)
			recvBuff_.erase();
		return false;
	}

	if (recvBuff_.readableBytes() > bufferNotifySize_ * 5)
	{
		TRACE("Too much data !!!!!\n");
		recvBuff_.erase();
		playQueue_.clear();
		return false;
	}

	int matchSize = bufferNotifySize_;
	recvBuff_.pushBack(data, len);

	while (recvBuff_.readableBytes() >= matchSize)
	{
		if (playQueue_.size() > 3)
		{
//			TRACE("Tooooooooo much in queue !!!!!\n");
			break;
		}
		BYTE* buf = (BYTE*)recvBuff_.beginRead();
		Buffer tmp;
		tmp.pushBack((char*)buf, matchSize);
		playQueue_.putBack(tmp);
		recvBuff_.eraseFront(matchSize);
	}
	
	return true;
}

void AudioPlay::reset()
{
	playQueue_.clear();
	recvBuff_.erase();
}

int AudioPlay::setVolume(int vol)
{
	if (!started_ || pDSB8_ == NULL)
		return vol_;

	vol_ = vol;

	if (vol_ < 0)
		vol_ = 0;
	if (vol_ >= VOL_RANG)
		vol_ = VOL_RANG-1;

	pDSB8_->SetVolume(volTable[vol_]);

	return vol_;
}

int AudioPlay::getVolume()
{
	return vol_;
}

void AudioPlay::loop()
{
	//设置提醒

	DSBPOSITIONNOTIFY DSBPositionNotify[MAX_AUDIO_BUF];
	HANDLE ev = CreateEvent(NULL, FALSE, FALSE, NULL);
	for (int i = 0; i<MAX_AUDIO_BUF; i++)
	{
		DSBPositionNotify[i].dwOffset = i*bufferNotifySize_;
		DSBPositionNotify[i].hEventNotify = ev;
	}
	pDSN_->SetNotificationPositions(MAX_AUDIO_BUF, DSBPositionNotify);

	LPVOID buf = NULL;
	DWORD  buf_len = 0;
	DWORD obj = WAIT_OBJECT_0;
	DWORD offset = bufferNotifySize_ ;
	DWORD offsetPlay = 0;
	DWORD totalSize = bufferNotifySize_*MAX_AUDIO_BUF;

	mute_ = false;
	bool empty = false;

	Buffer tmp;

	if (SUCCEEDED(pDSB8_->Lock(0, totalSize, &buf, &buf_len, NULL, NULL, 0)))
	{
		memset(buf, 0, buf_len);
		pDSB8_->Unlock(buf, buf_len, NULL, 0);
	}

	pDSB8_->SetCurrentPosition(0);

	HRESULT hr = pDSB8_->Play(0, 0, DSBPLAY_LOOPING);
	Sleep(ptime_);
	pDSB8_->SetVolume(volTable[vol_]);

	started_ = true;
	while (started_)
	{
		if (!playQueue_.getFront(tmp, ptime_ / 2))
		{
			pDSB8_->GetCurrentPosition(&offsetPlay, NULL);
			DWORD doffset = (offset - offsetPlay + totalSize) % totalSize;
			if (doffset >= bufferNotifySize_)
				continue;

			//			TRACE("offset=%d, offsetPlay=%d : 静音!!!!\n", offset, offsetPlay);
			tmp.erase();
			tmp.pushBack((unsigned char)0, bufferNotifySize_);		//静音
		}
		else
		{
			pDSB8_->GetCurrentPosition(&offsetPlay, NULL);
			DWORD doffset = (offsetPlay - offset + totalSize) % totalSize;
			if (doffset <= bufferNotifySize_)
			{
				//				TRACE("offset=%d, offsetPlay=%d : 等待!!!!\n", offset, offsetPlay);
				::ResetEvent(ev);
				obj = WaitForSingleObject(ev, INFINITE);
				if ((obj < WAIT_OBJECT_0) || (obj >= WAIT_OBJECT_0 + MAX_AUDIO_BUF))
					continue;
			}

		}

		if (FAILED(pDSB8_->Lock(offset, bufferNotifySize_, &buf, &buf_len, NULL, NULL, 0)))
		{
			TRACE("Lock in %d failed.\n", offset);
			ResetEvent(ev);
			obj = WaitForSingleObject(ev, INFINITE);
			if ((obj < WAIT_OBJECT_0) || (obj >= WAIT_OBJECT_0 + MAX_AUDIO_BUF))
				continue;

			if (FAILED(pDSB8_->Lock(offset, bufferNotifySize_, &buf, &buf_len, NULL, NULL, 0)))
			{
				TRACE("Lock in %d failed again !!!!!!!!\n", offset);
				continue;
			}

		}
		memcpy(buf, tmp.beginRead(), tmp.readableBytes());
		pDSB8_->Unlock(buf, buf_len, NULL, 0);

		assert(buf_len == bufferNotifySize_);
		offset = (offset + buf_len) % (bufferNotifySize_*MAX_AUDIO_BUF);
	}

	pDSB8_->Stop();
	pDSB8_->Release();

	pDSN_->Release();

	//	for (int i = 0; i<MAX_AUDIO_BUF; i++)
	CloseHandle(ev);

}
