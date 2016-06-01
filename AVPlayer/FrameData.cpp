#include "FrameData.h"

FrameData::FrameData()
	: width_(0)
	, height_(0)
	, size_(0)
	, data_(NULL)
	, tm_(0)
{
}

FrameData::FrameData(AVCodecID codec, AVPixelFormat format, int width, int height, int size, const BYTE * data)
	: width_(width)
	, height_(height)
	, size_(0)
	, data_(NULL)
	, tm_(0)
{
	if (size > 0 && data != NULL)
	{
		size_ = size;
		data_ = new BYTE[size_];
		memcpy(data_, data, size_);
	}
}

FrameData::FrameData(const FrameData & p)
	: data_(NULL)
{
	take(p);
}


FrameData::~FrameData()
{
	reset();
}

void FrameData::take(const FrameData & p)
{
	width_ = p.width_;
	height_ = p.height_;
	size_ = p.size_;
	tm_ = p.tm_;

	delete[] data_;
	data_ = p.data_;

	p.data_ = NULL;
	p.size_ = 0;
}

void FrameData::reset()
{
	delete[] data_;
	data_ = NULL;
	size_ = 0;
}
