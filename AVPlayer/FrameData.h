#pragma once

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
};

#include "base/timestamp.h"

static const AVPixelFormat GDI_FORMAT = AV_PIX_FMT_BGRA;

class FrameData
{
public:
	FrameData();
	FrameData(AVCodecID codec, AVPixelFormat format, int width, int height, int size, const BYTE* data);
	FrameData(const FrameData& p);

	~FrameData();

	FrameData& operator=(const FrameData& rhs)
	{
		take(rhs);
		return *this;
	}

	void reset();

private:
	void take(const FrameData& p);

public:
	mutable int width_;
	mutable int height_;
	mutable int size_;
	mutable BYTE* data_;
	mutable double tm_;
};


typedef boost::function<void(FrameData)> FrameHandler;
