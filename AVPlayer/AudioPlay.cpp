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
}


AudioPlay::~AudioPlay()
{
	stop();
	lpDSound_->Release();
}

bool AudioPlay::start(UINT samplesPerSec, UINT channels, unsigned char bitsPerSample, int ptime)
{
	stop();

	ptime_ = ptime*2;

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

	//������������������
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

	//��ȡ�ӿ�
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

	if (!res || !thread_.start(boost::bind(&AudioPlay::loop, this)))
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

void AudioPlay::inputPcm(const char * data, int len)
{
	if (!isStart() || mute_)
	{
		if (recvBuff_.readableBytes() > 0)
			recvBuff_.erase();
		return;
	}

	if (recvBuff_.readableBytes() > bufferNotifySize_ * 10)
	{
		TRACE("Tooooooooo much data !!!!!\n");
		recvBuff_.erase();
	}

	int matchSize = bufferNotifySize_;
	recvBuff_.pushBack(data, len, true);
	if (recvBuff_.readableBytes() < matchSize)
		return;

	Buffer tmp;
	BYTE* buf = (BYTE*)recvBuff_.beginRead();
	tmp.pushBack((char*)buf, matchSize);

	if (playQueue_.size() > 10)
	{
		TRACE("Tooooooooo much in queue !!!!!\n");
		playQueue_.clear();
	}
	playQueue_.putBack(tmp);

	recvBuff_.eraseFront(matchSize);
}

void AudioPlay::reset()
{
	playQueue_.clear();
	recvBuff_.erase();
}

void AudioPlay::loop()
{
	//��������

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
	DWORD offset = bufferNotifySize_ * 2;
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

	started_ = true;
	while (started_)
	{
		if (!playQueue_.getFront(tmp, ptime_ / 2))
		{
			pDSB8_->GetCurrentPosition(&offsetPlay, NULL);
			DWORD doffset = (offset - offsetPlay + totalSize) % totalSize;
			if (doffset >= bufferNotifySize_)
				continue;

			//			TRACE("offset=%d, offsetPlay=%d : ����!!!!\n", offset, offsetPlay);
			tmp.erase();
			tmp.pushBack((unsigned char)0, bufferNotifySize_, true);		//����
		}
		else
		{
			pDSB8_->GetCurrentPosition(&offsetPlay, NULL);
			DWORD doffset = (offsetPlay - offset + totalSize) % totalSize;
			if (doffset <= bufferNotifySize_)
			{
				//				TRACE("offset=%d, offsetPlay=%d : �ȴ�!!!!\n", offset, offsetPlay);
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