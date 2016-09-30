#pragma once

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
};

#include "base/timestamp.h"

static const AVPixelFormat GDI_FORMAT = AV_PIX_FMT_BGRA;

enum FrameType
{
	FRAME_VIDEO, 			
	FRAME_AUDIO,
};

class FrameData
{
public:
	FrameData();
	FrameData(FrameType type, int width, int height, int size, const BYTE* data);
	FrameData(const FrameData& p);

	~FrameData();

	FrameData& operator=(const FrameData& rhs)
	{
		take(rhs);
		return *this;
	}

	void reset();

	bool toFileJpg(const char* fipath) const;
	bool toFilePng(const char* fipath) const;
	bool toFile(const char* fmt, const char* fipath) const;

private:
	void take(const FrameData& p);

public:
	mutable FrameType type_;
	mutable int width_;
	mutable int height_;
	mutable int size_;
	mutable BYTE* data_;
	mutable int64_t tm_;
};


typedef std::function<bool(FrameData)> FrameHandler;
