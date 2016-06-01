// DrawWndD3d.cpp : 实现文件
//

#include "stdafx.h"
#include "AVPlayer.h"
#include "DrawWndD3d.h"

#include <d3dx9math.h>


#define LOAD_YUV420P 0
#define USAGE_TEXTURE

typedef struct
{
	FLOAT       x, y, z;      // vertex untransformed position
//	FLOAT       rhw;      // vertex untransformed position
	FLOAT       tu, tv;     // texture relative coordinates
} CUSTOMVERTEX;

// Custom flexible vertex format (FVF), which describes custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_TEX1)

// CDrawWndD3d

IMPLEMENT_DYNAMIC(CDrawWndD3d, CWnd)

CDrawWndD3d::CDrawWndD3d()
	: cb_(NULL)
	, scale_(100)
	, xPos_(0.0f)
	, yPos_(0.0f)
	, width_(800)
	, height_(600)
	, rotation_(0.0f)
	, pDirect3D_(NULL)
	, pDirect3DDevice_(NULL)
	, pDirect3DSurfaceRender_(NULL)
	, pDirect3DSurfaceBk_(NULL)
	, pDirect3DSprite_(NULL)
	, pDirect3DTexture_(NULL)
{
}

CDrawWndD3d::~CDrawWndD3d()
{
}

void CDrawWndD3d::Cleanup()
{
	if (pDirect3DTexture_)
		pDirect3DTexture_->Release(), pDirect3DTexture_ = NULL;

	if (pDirect3DSprite_)
		pDirect3DSprite_->Release(), pDirect3DSprite_ = NULL;

	if (pDirect3DSurfaceRender_)
		pDirect3DSurfaceRender_->Release(), pDirect3DSurfaceRender_ = NULL;

	if (pDirect3DSurfaceBk_)
		pDirect3DSurfaceBk_->Release(), pDirect3DSurfaceBk_ = NULL;

	if (pDirect3DDevice_)
		pDirect3DDevice_->Release(), pDirect3DDevice_ = NULL;

	if (pDirect3D_)
		pDirect3D_->Release(), pDirect3D_ = NULL;
}

BOOL CDrawWndD3d::CreateDevice()
{
	ZeroMemory(&d3dpp_, sizeof(d3dpp_));
	d3dpp_.Windowed = TRUE;
	d3dpp_.BackBufferCount = 1;
	d3dpp_.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp_.BackBufferFormat = D3DFMT_UNKNOWN;

	HRESULT ret = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, this->GetSafeHwnd(),
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

void CDrawWndD3d::DrawFrame(const FrameData & f)
{
	assert(f.data_ != NULL && f.size_ > 0);
	assert(f.width_ > 0 && f.height_ > 0);

#ifdef USAGE_TEXTURE
	/// 使用Texture ///
	DrawTexture(f);
#else
	/// 使用Surface ///
	DrawSurface(f);
#endif // USAGE_TEXTURE

}

void CDrawWndD3d::ResetCoordinate()
{
	//使用Surface matScale_, matRotate_, matTrans_
	D3DXMatrixIdentity(&matSprite_);

	xPos_ = 0.0f;
	yPos_ = 0.0f;

	scale_ = 100;
	rotation_ = 0.0f;

	UpdateMatSprite();

	if (cb_)
		cb_->OnResetSize(width_, height_);
}

//////////////////// 使用Surface //////////////////////////

void CDrawWndD3d::RecalculateRect(BOOL render)
{
	rectSrc_.left = rectSrc_.top = 0;
	rectSrc_.right = width_;
	rectSrc_.bottom = height_;

	int WIDTH = width_*scale_ / 100;
	int HEIGHT = height_*scale_ / 100;

	GetClientRect(&rectDst_);
	int DW = (rectDst_.right - WIDTH) / 2;
	int DH = (rectDst_.bottom - HEIGHT) / 2;

	int dw = DW * 100 / scale_;
	int dh = DH * 100 / scale_;

	if (DW < 0)
	{
		rectSrc_.left = -dw;
		rectSrc_.right = rectSrc_.left+rectDst_.right * 100 / scale_;
	}
	else
	{
		rectDst_.left = DW;
		rectDst_.right = rectDst_.left + WIDTH;
	}

	if (DH < 0)
	{
		rectSrc_.top = -dh;
		rectSrc_.bottom = rectSrc_.top+rectDst_.bottom * 100 / scale_;
	}
	else
	{
		rectDst_.top = DH;
		rectDst_.bottom = rectDst_.top + HEIGHT;
	}
	
	/*
	rectDst_.left = DW;
	rectDst_.right = rectDst_.left + WIDTH;
	rectDst_.top = DH;
	rectDst_.bottom = rectDst_.top + HEIGHT;
	*/

	if (render)
	{
		RenderSurface();
	}
}

void CDrawWndD3d::ResetSurfaceBk()
{
	RECT r;
	GetClientRect(&r);
	if (!CreateSurface(r.right, r.bottom, &pDirect3DSurfaceBk_))
		return;

	RenderSurface();
}

void CDrawWndD3d::ResetSurface(int width, int height)
{
	width_ = width;
	height_ = height;
	if (!CreateSurface(width, height, &pDirect3DSurfaceRender_))
		return;;

	if (cb_)
		cb_->OnResetSize(width, height);
}

BOOL CDrawWndD3d::CreateSurface(int width, int height, IDirect3DSurface9** ppSurface)
{
	if (!pDirect3D_ || (!pDirect3DDevice_ && !CreateDevice()))
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

	RecalculateRect();

	return TRUE;
}

void CDrawWndD3d::DrawSurface(const FrameData & f)
{
	if (pDirect3DSurfaceRender_ == NULL || width_ != f.width_ || height_ != f.height_)
		ResetSurface(f.width_, f.height_);

	D3DLOCKED_RECT d3d_rect;
	HRESULT ret = pDirect3DSurfaceRender_->LockRect(&d3d_rect, NULL, D3DLOCK_DISCARD);
	if (FAILED(ret))
	{
//		LOGE("pDirect3DSurfaceRender_->LockRect failed !!");
		return;
	}

	BYTE* pSrc = f.data_;
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
	int pixel_w_size = width_ * 4;
	if (stride != pixel_w_size)
	{
		for (i = 0; i< height_; i++) 
		{
			memcpy(pDest, pSrc, pixel_w_size);
			pDest += stride;
			pSrc += pixel_w_size;
		}
	}
	else memcpy(pDest, pSrc, pixel_w_size*height_);
#endif

	pDirect3DSurfaceRender_->UnlockRect();

	RenderSurface();
}

void CDrawWndD3d::RenderSurface()
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

///////////////////////////////////////////////////////////

//////////////////// 使用Texture //////////////////////////

void CDrawWndD3d::ResetTexture(int width, int height)
{
	if (!pDirect3DDevice_ && !CreateDevice())
		return;

	if (pDirect3DTexture_)
		pDirect3DTexture_->Release(), pDirect3DTexture_ = NULL;

	width_ = width;
	height_ = height;

	UpdateMatSprite();

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
		return;
	}

	if (cb_)
		cb_->OnResetSize(width, height);
}

void CDrawWndD3d::DrawTexture(const FrameData & f)
{
	if (pDirect3DTexture_ == NULL || width_ != f.width_ || height_ != f.height_)
		ResetTexture(f.width_, f.height_);

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
	byte *pSrc = f.data_;
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
	int pixel_w_size = width_ * 4;
	if (stride != pixel_w_size)
	{
		for (i = 0; i< height_; i++)
		{
			memcpy(pDest, pSrc, pixel_w_size);
			pDest += stride;
			pSrc += pixel_w_size;
		}
	}
	else memcpy(pDest, pSrc, pixel_w_size*height_);
#endif

	pDirect3DTexture_->UnlockRect(0);

	RenderTexture();
}

void CDrawWndD3d::RenderTexture()
{
	if (pDirect3D_ == NULL || (pDirect3DDevice_ == NULL && !CreateDevice()))
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
		RECT r;
		GetClientRect(&r);
		D3DXVECTOR3 center(r.right / 2, r.bottom / 2 , 0);
		pDirect3DSprite_->Draw(pDirect3DTexture_, NULL, NULL, NULL, 0xFFFFFFFF);
		pDirect3DSprite_->End();
	}
	
	pDirect3DDevice_->EndScene();
	pDirect3DDevice_->Present(NULL, NULL, NULL, NULL);
}

void CDrawWndD3d::UpdateMatSprite(BOOL render)
{
	RECT r;
	GetClientRect(&r);

	if (r.right == 0 || r.bottom == 0)
		return;

	float ratio_w = width_ /(float)r.right; 
	float ratio_h = height_ / (float)r.bottom;

	float scale_x = (scale_ / 100.0f)*ratio_w;
	float scale_y = (scale_ / 100.0f)*ratio_h;

	float width = width_*scale_x;
	float height = height_*scale_y;

	D3DXVECTOR2 rotation_center(width / 2, height / 2);
	D3DXVECTOR2 scale_center(r.right/2+xPos_, r.bottom / 2+yPos_);

	float x = (r.right - width) / 2;
	float y = (r.bottom - height) / 2;

	D3DXMatrixTransformation2D(&matSprite_, 
		&scale_center, 0.0f,
		&D3DXVECTOR2(scale_x, scale_y),
		&rotation_center, rotation_,
		&D3DXVECTOR2(xPos_, yPos_));

	if (cb_)
		cb_->ReportParams(xPos_, yPos_, scale_/100.0f, rotation_, width, height, r);

	if (render)
		RenderTexture();
}

///////////////////////////////////////////////////////////


BEGIN_MESSAGE_MAP(CDrawWndD3d, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_COMMAND(IDC_ROTATE_ANGLE, &CDrawWndD3d::OnRotateAngle)
	ON_COMMAND(IDC_ZOOM_IN, &CDrawWndD3d::OnZoomIn)
	ON_COMMAND(IDC_ZOOM_OUT, &CDrawWndD3d::OnZoomOut)
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(IDC_INIT_SIZE, &CDrawWndD3d::OnInitSize)
END_MESSAGE_MAP()

// CDrawWndD3d 消息处理程序

int CDrawWndD3d::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	pDirect3D_ = ::Direct3DCreate9(D3D_SDK_VERSION);
	if (pDirect3D_ == NULL)
	{
		LOGE("Direct3DCreate9 failed !!");
		return -1;
	}

	ResetCoordinate();

	return 0;
}

void CDrawWndD3d::OnDestroy()
{
	CWnd::OnDestroy();
	// TODO: 在此处添加消息处理程序代码
	Cleanup();
}

void CDrawWndD3d::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码

#ifdef USAGE_TEXTURE
	UpdateMatSprite(TRUE);
#else
	/// 使用Surface ///
	ResetSurfaceBk();
#endif // USAGE_TEXTURE

}

void CDrawWndD3d::OnInitSize()
{
	// TODO: 在此添加命令处理程序代码
	ResetCoordinate();
}

void CDrawWndD3d::OnRotateAngle()
{
	// TODO: 在此添加命令处理程序代码
	rotation_ += D3DX_PI / 4;

#ifdef USAGE_TEXTURE
	UpdateMatSprite(TRUE);
#else
#endif // USAGE_TEXTURE
}

void CDrawWndD3d::OnZoomIn()
{
	// TODO: 在此添加命令处理程序代码
	int SPAN = (scale_<100)?5:10;
	scale_ += SPAN;
	if (scale_ > 500)
		scale_ = 500;

#ifdef USAGE_TEXTURE
	UpdateMatSprite(TRUE);
#else
	RecalculateRect(TRUE);
#endif // USAGE_TEXTURE
}

void CDrawWndD3d::OnZoomOut()
{
	// TODO: 在此添加命令处理程序代码
	int SPAN = (scale_<100) ? 5 : 10;
	scale_ -= SPAN;
	if (scale_ < 10)
		scale_ = 10;

#ifdef USAGE_TEXTURE
	UpdateMatSprite(TRUE);
#else
	RecalculateRect(TRUE);
#endif // USAGE_TEXTURE
}

BOOL CDrawWndD3d::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (zDelta > 0)
		OnZoomIn();
	else OnZoomOut();
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);

}

void CDrawWndD3d::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnRButtonUp(nFlags, point);
	OnRotateAngle();
}

void CDrawWndD3d::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnLButtonDown(nFlags, point);
	SetCapture();
	posMove_ = point;
}

void CDrawWndD3d::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnLButtonUp(nFlags, point);
	if (GetCapture() == this)
	{
		ReleaseCapture();
		xPos_ += (point.x - posMove_.x);
		yPos_ += (point.y - posMove_.y);
		UpdateMatSprite(TRUE);
	}
}

void CDrawWndD3d::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnMouseMove(nFlags, point);
	if (GetCapture() == this)
	{
		xPos_ += (point.x - posMove_.x);
		yPos_ += (point.y - posMove_.y);
		UpdateMatSprite(TRUE);
		posMove_ = point;
	}
}

