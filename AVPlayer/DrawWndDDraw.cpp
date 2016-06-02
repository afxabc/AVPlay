#include "stdafx.h"
#include "DrawWndDDraw.h"


CDrawWndDDraw::CDrawWndDDraw()
	: lpDirectDraw_(NULL)
	, lpSurface_(NULL)
	, lpBkSurface_(NULL)
	, lpClipper_(NULL)
{
}


CDrawWndDDraw::~CDrawWndDDraw()
{
}

void CDrawWndDDraw::Cleanup()
{
	if (lpSurface_)
		lpSurface_->Release(), lpSurface_ = NULL;

	if (lpBkSurface_)
		lpBkSurface_->Release(), lpBkSurface_ = NULL;

	if (lpClipper_)
		lpClipper_->Release(), lpClipper_ = NULL;

	if (lpDirectDraw_)
		lpDirectDraw_->Release(), lpDirectDraw_ = NULL;
}

BOOL CDrawWndDDraw::CreateDevice(HWND hwnd)
{
	if (DirectDrawCreate(NULL, &lpDirectDraw_, NULL) != DD_OK)
	{
		LOGE("DirectDrawCreate failed !!");
		return FALSE;
	}

	if (lpDirectDraw_->SetCooperativeLevel(hwnd, DDSCL_NORMAL) != DD_OK)
	{
		Cleanup();
		LOGE("SetCooperativeLevel failed !!");
		return FALSE;
	}

	DDSURFACEDESC desc;
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DDSD_CAPS;
	desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (lpDirectDraw_->CreateSurface(&desc, &lpSurface_, NULL) != DD_OK)
	{
		Cleanup();
		LOGE("CreateSurface failed !!");
		return FALSE;
	}

	if (lpDirectDraw_->CreateClipper(0, &lpClipper_, NULL) != DD_OK)
	{
		LOGE("CreateClipper failed !!");
		Cleanup();
		return FALSE;
	}

	if (lpClipper_->SetHWnd(0, hwnd) != DD_OK || lpSurface_->SetClipper(lpClipper_) != DD_OK)
	{
		LOGE("Clipper->SetHWnd failed !!");
		Cleanup();
		return FALSE;
	}

	/*
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize = sizeof(desc);
	if (lpSurface_->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL) == DD_OK)
	{
		memset(desc.lpSurface, 0, desc.lPitch * 4 * desc.dwHeight);
		lpSurface_->Unlock(NULL);
	}
	*/

	return TRUE;
}

void CDrawWndDDraw::UpdateCoordinate(float scale, float rotate, POINT pos, SIZE szFrm, SIZE szWnd)
{
	rect_.left = (szWnd.cx - szFrm.cx) / 2;
	rect_.right = rect_.left + szFrm.cx;
	rect_.top = (szWnd.cy - szFrm.cy) / 2;
	rect_.bottom = rect_.top + szFrm.cy;
	//	this->ClientToScreen(&rect_);
}

void CDrawWndDDraw::DrawFrame(const BYTE * pSrc, int width, int height)
{
	if (lpBkSurface_ == NULL && !ResetSurface(width_, height_))
		return;

	DDSURFACEDESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize = sizeof(desc);
	if (lpBkSurface_->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL) != DD_OK)
	{
		LOGE("lpBkSurface_->Lock failed !!");
		return;
	}

	//
	BYTE* pDest = (BYTE *)desc.lpSurface;
	int stride = desc.lPitch;
	unsigned long i = 0;

	//Copy Data
#if LOAD_YUV420P
	for (i = 0; i < height_; i++) {
		memcpy(pDest + i * stride, pSrc + i * width_, width_);
	}
	for (i = 0; i < height_ / 2; i++) {
		memcpy(pDest + stride * height_ + i * stride / 2, pSrc + width_ * height_ + width_ * height_ / 4 + i * width_ / 2, width_ / 2);
	}
	for (i = 0; i < height_ / 2; i++) {
		memcpy(pDest + stride * height_ + stride * height_ / 4 + i * stride / 2, pSrc + width_ * height_ + i * width_ / 2, width_ / 2);
	}
#else
	int pixel_w_size = width * 4;
	if (stride != pixel_w_size)
	{
		for (i = 0; i< height; i++)
		{
			memcpy(pDest, pSrc, pixel_w_size);
			pDest += stride;
			pSrc += pixel_w_size;
		}
	}
	else memcpy(pDest, pSrc, pixel_w_size*height);
#endif

	lpBkSurface_->Unlock(NULL);

	Render();
}

void CDrawWndDDraw::Render()
{
	if (lpSurface_ == NULL)
		return;

	if (lpBkSurface_ != NULL)
	{
		DDBLTFX  ddbltfx;
		memset(&ddbltfx, 0, sizeof(ddbltfx));
		ddbltfx.dwSize = sizeof(ddbltfx);
		ddbltfx.dwROP = SRCCOPY;
		ddbltfx.dwRotationAngle = 0;
		ddbltfx.dwDDFX = DDBLTFX_NOTEARING;
		lpSurface_->Blt(&rect_, lpBkSurface_, NULL, DDBLT_WAIT, &ddbltfx);// | DDBLT_ROTATIONANGLE | DDBLT_DDFX
	}
}

BOOL CDrawWndDDraw::ResetSurface(int width, int height)
{
	if (!lpDirectDraw_)
		return FALSE;

	if (lpBkSurface_)
		lpBkSurface_->Release(), lpBkSurface_ = NULL;

	DDSURFACEDESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	width_ = desc.dwWidth = width;
	height_ = desc.dwHeight = height;
	desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	if (lpDirectDraw_->CreateSurface(&desc, &lpBkSurface_, NULL) != DD_OK)
	{
		LOGW("DDSCAPS_VIDEOMEMORY failed, try DDSCAPS_SYSTEMMEMORY !");
		desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		if (lpDirectDraw_->CreateSurface(&desc, &lpBkSurface_, NULL) != DD_OK)
		{
			LOGE("DDSCAPS_SYSTEMMEMORY failed !!!");
			lpBkSurface_ = NULL;
			return FALSE;
		}
	}

	return TRUE;
}

