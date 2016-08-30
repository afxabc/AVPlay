#include "stdafx.h"
#include "DrawWndDDraw.h"

CDrawWndDDraw::CDrawWndDDraw(CWnd* wnd)
	: rectWnd_(wnd)
	, szWnd_(0, 0)
	, szFrm_(0, 0)
	, rotate_(ROTATION_0)
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

void CDrawWndDDraw::UpdateCoordinate(float scale, ROTATIONTYPE rotate, POINT pos, const FrameData& frm, HWND hwnd)
{
	rotate_ = rotate;
	CSize szFrm(frm.width_, frm.height_);
	if (rotate == ROTATION_90 || rotate == ROTATION_270)
	{
		int tmp = szFrm.cx;
		szFrm.cx = szFrm.cy;
		szFrm.cy = tmp;
	}

	RECT r;
	::GetClientRect(hwnd, &r);
	CSize szWnd(r.right, r.bottom);

	float WIDTH = (float)szFrm.cx*scale;
	float HEIGHT = (float)szFrm.cy*scale;

	rect_.left = (szWnd.cx - WIDTH) / 2;
	rect_.left += pos.x;
	rect_.right = rect_.left + WIDTH;

	rect_.top = (szWnd.cy - HEIGHT) / 2;
	rect_.top += pos.y;
	rect_.bottom = rect_.top + HEIGHT;

	if (szWnd_ != szWnd)
		ResetSurfaceBk(szWnd);

	if (szFrm_ != szFrm)
	{
		szFrm_ = szFrm;
		ResetSurfaceFrm(frm.width_, frm.height_, frm.data_);
	}
}

void CDrawWndDDraw::DrawFrame(const BYTE * pSrc, int width, int height)
{
	if (lpSurfaceFrm_ == NULL)
	{
		BOOL ret = FALSE;
		if (rotate_ == ROTATION_90 || rotate_ == ROTATION_270)
			ret = ResetSurfaceFrm(height, width);
		else ret = ResetSurfaceFrm(width, height);

		if (!ret)
			return;
	}

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
	int i = 0;
	int j = 0;

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
	if (rotate_ == ROTATION_0 || rotate_ == ROTATION_180)
	{
		if (rotate_ == ROTATION_180)
		{
			pSrc = pSrc + pixel_w_size*(height - 1);
			pixel_w_size  = -pixel_w_size;
		}

		for (i = 0; i< height; i++)
		{
			memcpy(pDest, pSrc, abs(pixel_w_size));
			pDest += stride;
			pSrc += pixel_w_size;
		}
	}
	else
	{
		if (rotate_ == ROTATION_90)
		{
			pSrc = pSrc + pixel_w_size*(height - 1);
			pixel_w_size  = -pixel_w_size;
		}

		for (i = 0; i < width; i++)
		{
			for (j = 0; j < height; j++)
			{
				memcpy(pDest+j*4, pSrc+ pixel_w_size*j+i*4, 4);
			}
			pDest += stride;
		}

	}
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
	{
		rectWnd_->GetClientRect(&rect);
		rectWnd_->ClientToScreen(&rect);
	}
	lpSurface_->Blt(&rect, lpSurfaceBk_, NULL, DDBLT_WAIT, &ddbltfx);
}

BOOL CDrawWndDDraw::ResetSurfaceBk(const SIZE & szWnd)
{
	if (!ResetSurface(szWnd.cx, szWnd.cy, &lpSurfaceBk_))
		return FALSE;
	lpSurfaceBk_->SetClipper(lpClipper_);
	szWnd_ = szWnd;
	return TRUE;
}

BOOL CDrawWndDDraw::ResetSurfaceFrm(int width, int height, LPBYTE data)
{
	if (!ResetSurface(szFrm_.cx, szFrm_.cy, &lpSurfaceFrm_))
		return FALSE;

	if (data != NULL)
		DrawFrame(data, width, height);

	return TRUE;
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

