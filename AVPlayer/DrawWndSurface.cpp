#include "stdafx.h"
#include "DrawWndSurface.h"

CDrawWndSurface::CDrawWndSurface()
	: pDirect3D_(NULL)
	, pDirect3DDevice_(NULL)
	, pDirect3DSurfaceRender_(NULL)
	, pDirect3DSurfaceBk_(NULL)
	, szSurfaceBk_(0, 0)
{
}

CDrawWndSurface::~CDrawWndSurface()
{
	Cleanup();
}

void CDrawWndSurface::Cleanup()
{
	if (pDirect3DSurfaceRender_)
		pDirect3DSurfaceRender_->Release(), pDirect3DSurfaceRender_ = NULL;

	if (pDirect3DSurfaceBk_)
		pDirect3DSurfaceBk_->Release(), pDirect3DSurfaceBk_ = NULL;

	if (pDirect3DDevice_)
		pDirect3DDevice_->Release(), pDirect3DDevice_ = NULL;

	if (pDirect3D_)
		pDirect3D_->Release(), pDirect3D_ = NULL;
}

BOOL CDrawWndSurface::CreateDevice(HWND hwnd)
{
	if (!pDirect3D_)
	{
		pDirect3D_ = ::Direct3DCreate9(D3D_SDK_VERSION);
		if (pDirect3D_ == NULL)
		{
			LOGE("Direct3DCreate9 failed !!");
			return FALSE;
		}
	}
	
	ZeroMemory(&d3dpp_, sizeof(d3dpp_));
	d3dpp_.Windowed = TRUE;
	d3dpp_.BackBufferCount = 1;
	d3dpp_.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp_.BackBufferFormat = D3DFMT_UNKNOWN;

	HRESULT ret = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp_, &pDirect3DDevice_);
	if (FAILED(ret))
	{
		LOGE("pDirect3D_->CreateDevice failed !!");
		//		Cleanup();
		return FALSE;
	}

	pDirect3DDevice_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	return TRUE;
}

void CDrawWndSurface::UpdateCoordinate(float scale, ROTATIONTYPE rotate, POINT pos, SIZE szFrm, SIZE szWnd)
{
	if (pDirect3DSurfaceBk_ == NULL || szSurfaceBk_ != szWnd)
	{
		if (ResetSurfaceBk(szWnd.cx, szWnd.cy))
			szSurfaceBk_ = szWnd;
	}

	rectSrc_.left = rectSrc_.top = 0;
	rectSrc_.right = szFrm.cx;
	rectSrc_.bottom = szFrm.cy;

	float WIDTH = szFrm.cx*scale;
	float HEIGHT = szFrm.cy*scale;

	rectDst_.left = rectDst_.top = 0;
	rectDst_.right = szWnd.cx;
	rectDst_.bottom = szWnd.cy;

	float DW = (rectDst_.right - WIDTH) / 2 + pos.x;
	float DH = (rectDst_.bottom - HEIGHT) / 2 + pos.y;

	float dw = DW / scale;
	float dh = DH / scale;

	if (DW < 0)
	{
		rectSrc_.left = -dw;
		rectSrc_.right = rectSrc_.left + (float)rectDst_.right / scale;
	}
	else
	{
		rectDst_.left = DW;
		rectDst_.right = rectDst_.left + WIDTH;
	}

	if (DH < 0)
	{
		rectSrc_.top = -dh;
		rectSrc_.bottom = rectSrc_.top + (float)rectDst_.bottom / scale;
	}
	else
	{
		rectDst_.top = DH;
		rectDst_.bottom = rectDst_.top + HEIGHT;
	}

	Render();
}

void CDrawWndSurface::DrawFrame(const BYTE * pSrc, int width, int height)
{
	if (pDirect3DSurfaceRender_ == NULL && !ResetSurfaceRender(width, height))
		return;

	D3DLOCKED_RECT d3d_rect;
	HRESULT ret = pDirect3DSurfaceRender_->LockRect(&d3d_rect, NULL, D3DLOCK_DISCARD);
	if (FAILED(ret))
		return;

	BYTE* pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
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

	pDirect3DSurfaceRender_->UnlockRect();

	Render();
}

BOOL CDrawWndSurface::ResetSurfaceBk(int width, int height)
{
	return CreateSurface(width, height, &pDirect3DSurfaceBk_);
}

BOOL CDrawWndSurface::ResetSurfaceRender(int width, int height)
{
	return CreateSurface(width, height, &pDirect3DSurfaceRender_);
}

BOOL CDrawWndSurface::CreateSurface(int width, int height, IDirect3DSurface9 ** ppSurface)
{
	if (!pDirect3DDevice_)
		return FALSE;

	if (*ppSurface)
		(*ppSurface)->Release(), *ppSurface = NULL;

	D3DFORMAT format;
#if LOAD_YUV420P
	format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');
#else
	format = D3DFMT_X8R8G8B8;
#endif

	HRESULT ret;
	/*
	ret = pDirect3DDevice_->CreateOffscreenPlainSurface(
	width, height,
	format,
	D3DPOOL_DEFAULT,
	ppSurface,
	NULL);
	*/
	int type = D3DMULTISAMPLE_10_SAMPLES;
	do
	{
		ret = pDirect3DDevice_->CreateRenderTarget(
			width, height,
			format,
			(D3DMULTISAMPLE_TYPE)type,
			0,
			TRUE,
			ppSurface,
			NULL);
		if (type > 0)
			type--;
		else break;
	} while (FAILED(ret));

	if (FAILED(ret))
	{
		LOGE("CreateSurface failed !!");
		return FALSE;
	}
	LOGW("CreateSurface D3DMULTISAMPLE_TYPE = %d", type);

	return TRUE;
}

void CDrawWndSurface::Render()
{
	if (pDirect3DDevice_ == NULL)
		return;

	//	pDirect3DDevice_->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	pDirect3DDevice_->BeginScene();

	if (pDirect3DSurfaceBk_ != NULL)
	{
		pDirect3DDevice_->SetRenderTarget(0, pDirect3DSurfaceBk_);
		pDirect3DDevice_->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

		if (pDirect3DSurfaceRender_ != NULL)
		{
			HRESULT ret = pDirect3DDevice_->StretchRect(pDirect3DSurfaceRender_, &rectSrc_, pDirect3DSurfaceBk_, &rectDst_, D3DTEXF_LINEAR);
			if (FAILED(ret))
			{
				LOGE("pDirect3DDevice_->StretchRect return %x !!", ret);
				return;
			}
		}

		IDirect3DSurface9 * pBackBuffer = NULL;
		pDirect3DDevice_->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
		pDirect3DDevice_->StretchRect(pDirect3DSurfaceBk_, NULL, pBackBuffer, NULL, D3DTEXF_NONE);
	}

	pDirect3DDevice_->EndScene();
	//	pDirect3DDevice_->Present(&rectSrc_, &rectDst_, NULL, NULL);
	pDirect3DDevice_->Present(NULL, NULL, NULL, NULL);
}
