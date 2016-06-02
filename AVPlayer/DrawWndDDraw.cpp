#include "stdafx.h"
#include "DrawWndDDraw.h"


CDrawWndDDraw::CDrawWndDDraw(CWnd* wnd)
	: rectWnd_(wnd)
	, szWnd_(0, 0)
	, lpDirectDraw_(NULL)
	, lpSurface_(NULL)
	, lpSurfaceBk_(NULL)
	, lpSurfaceFrm_(NULL)
	, lpClipper_(NULL)
{
}


CDrawWndDDraw::~CDrawWndDDraw()
{
	Cleanup();
}

void CDrawWndDDraw::Cleanup()
{
	if (lpSurface_)
		lpSurface_->Release(), lpSurface_ = NULL;

	if (lpSurfaceBk_)
		lpSurfaceBk_->Release(), lpSurfaceBk_ = NULL;

	if (lpSurfaceFrm_)
		lpSurfaceFrm_->Release(), lpSurfaceFrm_ = NULL;

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

	return TRUE;
}

void CDrawWndDDraw::UpdateCoordinate(float scale, float rotate, POINT pos, SIZE szFrm, SIZE szWnd)
{
	float WIDTH = szFrm.cx*scale;
	float HEIGHT = szFrm.cy*scale;

	rect_.left = (szWnd.cx - WIDTH) / 2;
	rect_.left += pos.x;
	rect_.right = rect_.left + WIDTH;

	rect_.top = (szWnd.cy - HEIGHT) / 2;
	rect_.top += pos.y;
	rect_.bottom = rect_.top + HEIGHT;

	if (szWnd_ != szWnd)
		ResetSurfaceBk(szWnd);
}

void CDrawWndDDraw::DrawFrame(const BYTE * pSrc, int width, int height)
{
	if (lpSurfaceFrm_ == NULL && !ResetSurfaceFrm(width, height))
		return;

	DDSURFACEDESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize = sizeof(desc);
	if (lpSurfaceFrm_->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL) != DD_OK)
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

	lpSurfaceFrm_->Unlock(NULL);

	Render();
}

void CDrawWndDDraw::Render()
{
	if (lpSurface_ == NULL || lpSurfaceBk_ == NULL)
		return;

	DDBLTFX  ddbltfx;
	memset(&ddbltfx, 0, sizeof(ddbltfx));
	ddbltfx.dwSize = sizeof(ddbltfx);
	ddbltfx.dwFillColor = RGB(0, 0, 0); 
	lpSurfaceBk_->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);

	if (lpSurfaceFrm_ != NULL)
	{
		memset(&ddbltfx, 0, sizeof(ddbltfx));
		ddbltfx.dwSize = sizeof(ddbltfx);
		ddbltfx.dwROP = SRCCOPY;
		ddbltfx.dwRotationAngle = 0;
		ddbltfx.dwDDFX = DDBLTFX_NOTEARING;
		lpSurfaceBk_->Blt(&rect_, lpSurfaceFrm_, NULL, DDBLT_WAIT, &ddbltfx);// | DDBLT_ROTATIONANGLE | DDBLT_DDFX
	}

	ddbltfx.dwROP = SRCCOPY;
	ddbltfx.dwDDFX = DDBLTFX_NOTEARING;
	RECT rect = rect_;
	if (rectWnd_)
		rectWnd_->ClientToScreen(&rect);
	lpSurface_->Blt(&rect, lpSurfaceBk_, NULL, DDBLT_WAIT, &ddbltfx);
}

BOOL CDrawWndDDraw::ResetSurfaceBk(const SIZE & szWnd)
{
	if (!ResetSurface(szWnd.cx, szWnd.cy, &lpSurfaceBk_))
		return FALSE;
	szWnd_ = szWnd;
	return TRUE;
}

BOOL CDrawWndDDraw::ResetSurfaceFrm(int width, int height)
{
	return ResetSurface(width, height, &lpSurfaceFrm_);
}

BOOL CDrawWndDDraw::ResetSurface(int width, int height, LPDIRECTDRAWSURFACE * ppSurface)
{
	if (!lpDirectDraw_)
		return FALSE;

	if (*ppSurface)
		(*ppSurface)->Release(), *ppSurface = NULL;

	DDSURFACEDESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	desc.dwWidth = width;
	desc.dwHeight = height;
	desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	if (lpDirectDraw_->CreateSurface(&desc, ppSurface, NULL) != DD_OK)
	{
		LOGW("DDSCAPS_VIDEOMEMORY failed, try DDSCAPS_SYSTEMMEMORY !");
		desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		if (lpDirectDraw_->CreateSurface(&desc, ppSurface, NULL) != DD_OK)
		{
			LOGE("DDSCAPS_SYSTEMMEMORY failed !!!");
			*ppSurface = NULL;
			return FALSE;
		}
	}

	return TRUE;
}

