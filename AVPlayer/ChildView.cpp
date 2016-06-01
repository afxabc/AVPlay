// ChildView.cpp : CChildView 类的实现
//

#include "stdafx.h"
#include "ChildView.h"

#include "base\buffer.h"
// CChildView

CChildView::CChildView()
	: cb_(NULL)
	, width_(100)
	, height_(100)
	, lpDirectDraw_(NULL)
	, lpSurface_(NULL)
	, lpBkSurface_(NULL)
	, lpClipper_(NULL)
{
}

CChildView::~CChildView()
{
	Cleanup();
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()



// CChildView 消息处理程序

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	return TRUE;
}

void CChildView::Cleanup()
{	if (lpSurface_)		lpSurface_->Release(), lpSurface_ = NULL;	if (lpBkSurface_)		lpBkSurface_->Release(), lpBkSurface_ = NULL;	if (lpClipper_)		lpClipper_->Release(), lpClipper_ = NULL;	if (lpDirectDraw_)		lpDirectDraw_->Release(), lpDirectDraw_ = NULL;
}

void CChildView::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	Cleanup();
}

int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码

	if (DirectDrawCreate(NULL, &lpDirectDraw_, NULL) != DD_OK)	{		LOGE("DirectDrawCreate failed !!");		return -1;	}	if (lpDirectDraw_->SetCooperativeLevel(GetSafeHwnd(), DDSCL_NORMAL) != DD_OK)	{		Cleanup();		LOGE("SetCooperativeLevel failed !!");		return -1;	}
	DDSURFACEDESC desc;
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DDSD_CAPS;
	desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;	if (lpDirectDraw_->CreateSurface(&desc, &lpSurface_, NULL) != DD_OK)	{		Cleanup();		LOGE("CreateSurface failed !!");		return -1;	}	if (lpDirectDraw_->CreateClipper(0, &lpClipper_, NULL) != DD_OK)	{		LOGE("CreateClipper failed !!");		Cleanup();		return -1;	}	if (lpClipper_->SetHWnd(0, GetSafeHwnd()) != DD_OK || lpSurface_->SetClipper(lpClipper_) != DD_OK)	{		LOGE("Clipper->SetHWnd failed !!");		Cleanup();		return -1;	}
	return 0;
}

void CChildView::ResetRect()
{	
	RECT r;
	GetClientRect(&r);
	rect_.left = (r.right - width_) / 2;
	rect_.right = rect_.left + width_;
	rect_.top = (r.bottom - height_) / 2;
	rect_.bottom = rect_.top + height_;
	this->ClientToScreen(&rect_);
}

void CChildView::ResetSurface(int width, int height)
{
	if (!lpDirectDraw_)
		return;
	if (lpBkSurface_)		lpBkSurface_->Release(), lpBkSurface_ = NULL;
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
			return;
		}
	}

	if (cb_)
		cb_->OnResetSize(width, height);

}

void CChildView::DrawFrame(const FrameData & f)
{
	assert(f.data_ != NULL && f.size_ > 0);

	if (lpBkSurface_ == NULL || width_ != f.width_ || height_ != f.height_)
		ResetSurface(f.width_, f.height_);

	DDSURFACEDESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.dwSize = sizeof(desc);
	if (lpBkSurface_->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL) != DD_OK)
	{
		LOGE("lpBkSurface_->Lock failed !!");
		return;
	}

	//
	memcpy(desc.lpSurface, f.data_, f.size_);
	lpBkSurface_->Unlock(NULL);

	DDBLTFX  ddbltfx;
	memset(&ddbltfx, 0, sizeof(ddbltfx));
	ddbltfx.dwSize = sizeof(ddbltfx);
	ddbltfx.dwROP = SRCCOPY;
	ddbltfx.dwRotationAngle = 0;
	ddbltfx.dwDDFX = DDBLTFX_NOTEARING;
	lpSurface_->Blt(&rect_, lpBkSurface_, NULL, DDBLT_WAIT, &ddbltfx);// | DDBLT_ROTATIONANGLE | DDBLT_DDFX
}
