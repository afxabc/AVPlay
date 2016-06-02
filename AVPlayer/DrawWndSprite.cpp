#include "stdafx.h"
#include "DrawWndSprite.h"

CDrawWndSprite::CDrawWndSprite()
	: pDirect3D_(NULL)
	, pDirect3DDevice_(NULL)
	, pDirect3DSprite_(NULL)
	, pDirect3DTexture_(NULL)
{
}

CDrawWndSprite::~CDrawWndSprite()
{
}

void CDrawWndSprite::Cleanup()
{
	if (pDirect3DTexture_)
		pDirect3DTexture_->Release(), pDirect3DTexture_ = NULL;

	if (pDirect3DSprite_)
		pDirect3DSprite_->Release(), pDirect3DSprite_ = NULL;

	if (pDirect3DDevice_)
		pDirect3DDevice_->Release(), pDirect3DDevice_ = NULL;

	if (pDirect3D_)
		pDirect3D_->Release(), pDirect3D_ = NULL;
}

BOOL CDrawWndSprite::CreateDevice(HWND hwnd)
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

void CDrawWndSprite::UpdateCoordinate(float scale, float rotate, POINT pos, SIZE szFrm, SIZE szWnd)
{
	float ratio_w = (float)szFrm.cx / (float)szWnd.cx;
	float ratio_h = (float)szFrm.cy / (float)szWnd.cy;

	float scale_x = scale*ratio_w;
	float scale_y = scale*ratio_h;

	float width = (float)szFrm.cx*scale_x;
	float height = (float)szFrm.cy*scale_y;

	D3DXVECTOR2 rotation_center(width / 2, height / 2);
	D3DXVECTOR2 scale_center(szWnd.cx / 2 + pos.x, szWnd.cy / 2 + pos.y);

	float x = (szWnd.cx - width) / 2;
	float y = (szWnd.cy - height) / 2;

	D3DXMatrixTransformation2D(&matSprite_,
		&scale_center, 0.0f,
		&D3DXVECTOR2(scale_x, scale_y),
		&rotation_center, rotate,
		&D3DXVECTOR2(pos.x, pos.y));
}

void CDrawWndSprite::DrawFrame(const BYTE * pSrc, int width, int height)
{
	if (pDirect3DTexture_ == NULL && !ResetTexture(width, height))
		return;

	D3DLOCKED_RECT d3d_rect;
	//Locks a rectangle on a texture resource.
	//And then we can manipulate pixel data in it.
	HRESULT ret = pDirect3DTexture_->LockRect(0, &d3d_rect, NULL, D3DLOCK_DISCARD);
	if (FAILED(ret))
	{
		LOGE("pDirect3DTexture_->LockRect !!");
		return;
	}

	// Copy pixel data to texture
	byte *pDest = (byte *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long i = 0;

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

	pDirect3DTexture_->UnlockRect(0);

	Render();
}

void CDrawWndSprite::Render()
{
	if (pDirect3DDevice_ == NULL)
		return;

	if (pDirect3DSprite_ == NULL && FAILED(D3DXCreateSprite(pDirect3DDevice_, &pDirect3DSprite_)))
		return;

	if (pDirect3DDevice_->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
	{
		pDirect3DDevice_->Reset(&d3dpp_);
		D3DXCreateSprite(pDirect3DDevice_, &pDirect3DSprite_);
	}

	pDirect3DDevice_->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	pDirect3DDevice_->BeginScene();

	if (pDirect3DTexture_ != NULL)
	{
		pDirect3DSprite_->Begin(D3DXSPRITE_ALPHABLEND);
		pDirect3DSprite_->SetTransform(&matSprite_);
		pDirect3DSprite_->Draw(pDirect3DTexture_, NULL, NULL, NULL, 0xFFFFFFFF);
		pDirect3DSprite_->End();
	}

	pDirect3DDevice_->EndScene();
	pDirect3DDevice_->Present(NULL, NULL, NULL, NULL);
}

BOOL CDrawWndSprite::ResetTexture(int width, int height)
{
	if (pDirect3DDevice_ == NULL)
		return FALSE;

	if (pDirect3DTexture_)
		pDirect3DTexture_->Release(), pDirect3DTexture_ = NULL;

	D3DFORMAT format;
#if LOAD_YUV420P
	format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');
#else
	format = D3DFMT_X8R8G8B8;
#endif
	HRESULT ret = pDirect3DDevice_->CreateTexture(width, height, 1,
		D3DUSAGE_AUTOGENMIPMAP,
		format,
		D3DPOOL_MANAGED,
		&pDirect3DTexture_, NULL);
	if (FAILED(ret))
	{
		LOGE("pDirect3DDevice_->CreateTexture !!");
		return FALSE;
	}

	return TRUE;
}
