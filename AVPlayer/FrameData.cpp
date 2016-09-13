#include "FrameData.h"

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
#include "libswscale\swscale.h"
#include "libswresample\swresample.h"
};

FrameData::FrameData()
	: type_(FRAME_VIDEO)
	, width_(0)
	, height_(0)
	, size_(0)
	, data_(NULL)
	, tm_(0)
{
}

FrameData::FrameData(FrameType type, int width, int height, int size, const BYTE * data)
	: type_(type)
	, width_(width)
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
	type_ = p.type_;
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

bool FrameData::toFile(const char * fmtstr, const char * fipath) const
{ 
	if (data_ == NULL || size_ == 0)
		return false;

	AVFormatContext* pFormatCtx = NULL;
	AVOutputFormat* pOutFormat = NULL;
	AVStream* pAVStream  = NULL;
	AVCodecContext* pCodecCtx = NULL;
	AVCodec* pCodec = NULL;

	AVFrame* pFrameYUV = NULL;
	AVFrame* pFrameRGB = NULL;

	AVPacket pkt;
	av_new_packet(&pkt, width_*height_ * 3);

	bool ret = false;

	pFormatCtx = avformat_alloc_context();
	//Guess format  
	pOutFormat = av_guess_format(fmtstr, NULL, NULL);
	pFormatCtx->oformat = pOutFormat;
	//Output URL  
	if (avio_open(&pFormatCtx->pb, fipath, AVIO_FLAG_READ_WRITE) < 0) 
	{
		LOGE("Couldn't open output file.");
		goto END;
	}

	pAVStream  = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream  == NULL) 
	{
		goto END;
	}
	pCodecCtx = pAVStream ->codec;

	av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);

	pCodecCtx->flags |= CODEC_FLAG_QSCALE;
	pCodecCtx->global_quality = (int)(FF_QP2LAMBDA * 10 + 0.5);

	pCodecCtx->codec_id = pOutFormat->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;

	pCodecCtx->width = width_;
	pCodecCtx->height = height_;

	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;


	avformat_write_header(pFormatCtx, NULL);
	//Output some information  
//	av_dump_format(pFormatCtx, 0, fipath, 1);

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec) 
	{
		LOGE("Codec not found.");
		goto END;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) 
	{
		LOGE("Could not open codec.");
		goto END;
	}

	SwsContext* swsContext = sws_getContext(width_, height_, AV_PIX_FMT_BGRA, width_, height_, pCodecCtx->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

	pFrameYUV = av_frame_alloc();
	size_t size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	uint8_t* picture_buf = (uint8_t*)av_malloc(size);
	avpicture_fill((AVPicture *)pFrameYUV, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	pFrameYUV->format = pCodecCtx->pix_fmt;
	pFrameYUV->width = width_;
	pFrameYUV->height = height_;

	pFrameRGB = av_frame_alloc();
	avpicture_fill((AVPicture *)pFrameRGB, data_, AV_PIX_FMT_BGRA, width_, height_);
	pFrameRGB->format = AV_PIX_FMT_BGRA;
	pFrameRGB->width = width_;
	pFrameRGB->height = height_;

	sws_scale(swsContext, pFrameRGB->data, pFrameRGB->linesize, 0, height_, pFrameYUV->data, pFrameYUV->linesize);
	

	sws_freeContext(swsContext);

	int got_picture = 0;

	//Encode  
	if (avcodec_encode_video2(pCodecCtx, &pkt, pFrameYUV, &got_picture) < 0)
	{
		LOGE("Encode Error.\n");
		goto END;
	}

	if (got_picture == 1) 
	{
		pkt.stream_index = pAVStream ->index;
		pkt.dts = pkt.pts = 10;
		av_write_frame(pFormatCtx, &pkt);
	}

	//Write Trailer  
	av_write_trailer(pFormatCtx);
	LOGI("Encode Successful.\n");

	ret = true;

END:
	av_free_packet(&pkt);

	if (pFrameRGB)
	{
		av_frame_free(&pFrameRGB);
	}
		
	if (pFrameYUV)
	{
		av_frame_free(&pFrameYUV);
		av_free(picture_buf);
	}

	if (pCodecCtx)
		avcodec_close(pCodecCtx);

	if (pFormatCtx)
	{
		avio_close(pFormatCtx->pb);
		avformat_free_context(pFormatCtx);
	}

	
	return ret;
}

bool FrameData::toFileJpg(const char * fipath) const
{
	return toFile("mjpeg", fipath);
}

bool FrameData::toFilePng(const char * fipath) const
{
	return false;
}
