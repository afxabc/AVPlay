// DrawWnd.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "AVPlayer.h"
#include "DrawWnd.h"

#include "DrawWndSurface.h"
#include "DrawWndSprite.h"
#include "DrawWndDDraw.h"

#define PI 3.1415926f
// CDrawWnd

IMPLEMENT_DYNAMIC(CDrawWnd, CWnd)

CDrawWnd::CDrawWnd()
	: cb_(NULL)
	, pHandle_(NULL)
	, scale_(100)
	, xPos_(0.0f)
	, yPos_(0.0f)
	, width_(801)
	, height_(601)
	, WIDTH_(801)
	, HEIGHT_(601)
	, rotation_(0.0f)
{
}

CDrawWnd::~CDrawWnd()
{
}

void CDrawWnd::DrawFrame(const FrameData & f)
{
	assert(f.data_ != NULL && f.size_ > 0);
	assert(f.width_ > 0 && f.height_ > 0);

	if (width_ != f.width_ || height_ != f.height_)
	{
		width_ = f.width_;
		height_ = f.height_;
		pHandle_->OnFrmSizeChange(f.width_, f.height_);
		UpdateCoordinate();
		if (cb_)
			cb_->OnResetSize(width_, height_);
	}

	pHandle_->DrawFrame(f.data_, f.width_, f.height_);
}

//////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CDrawWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(IDC_ROTATE_ANGLE, &CDrawWnd::OnRotateAngle)
	ON_COMMAND(IDC_ZOOM_IN, &CDrawWnd::OnZoomIn)
	ON_COMMAND(IDC_ZOOM_OUT, &CDrawWnd::OnZoomOut)
	ON_COMMAND(IDC_INIT_SIZE, &CDrawWnd::OnInitSize)
END_MESSAGE_MAP()

// CDrawWnd ��Ϣ�������

int CDrawWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  �ڴ������ר�õĴ�������

//	pHandle_ = new CDrawWndSurface();
//	pHandle_ = new CDrawWndSprite();
	pHandle_ = new CDrawWndDDraw();

	return 0;
}

void CDrawWnd::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: �ڴ˴������Ϣ����������
	pHandle_->Cleanup();
	delete pHandle_;
	pHandle_ = NULL;
}

void CDrawWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: �ڴ˴������Ϣ����������
	if (pHandle_ == NULL)
		return;

	if (!pHandle_->IsValid())
		pHandle_->CreateDevice(GetSafeHwnd());

	WIDTH_ = cx;
	HEIGHT_ = cy;
	UpdateCoordinate(TRUE);
}

BOOL CDrawWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (zDelta > 0)
		OnZoomIn();
	else OnZoomOut();
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CDrawWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CWnd::OnRButtonUp(nFlags, point);
	OnRotateAngle();
}

void CDrawWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CWnd::OnLButtonDown(nFlags, point);
	SetCapture();
	posMove_ = point;
}

void CDrawWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CWnd::OnLButtonUp(nFlags, point);
	if (GetCapture() == this)
	{
		ReleaseCapture();
		xPos_ += (point.x - posMove_.x);
		yPos_ += (point.y - posMove_.y);
		UpdateCoordinate();
	}
}

void CDrawWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CWnd::OnMouseMove(nFlags, point);
	if (GetCapture() == this)
	{
		xPos_ += (point.x - posMove_.x);
		yPos_ += (point.y - posMove_.y);
		UpdateCoordinate();
		posMove_ = point;
	}
}

void CDrawWnd::OnRotateAngle()
{
	// TODO: �ڴ���������������
	rotation_ += PI / 4;
	UpdateCoordinate();
}

void CDrawWnd::OnZoomIn()
{
	// TODO: �ڴ���������������
	int SPAN = (scale_<100) ? 5 : 10;
	scale_ += SPAN;
	if (scale_ > 500)
		scale_ = 500;
	UpdateCoordinate();
}

void CDrawWnd::OnZoomOut()
{
	// TODO: �ڴ���������������
	int SPAN = (scale_<100) ? 5 : 10;
	scale_ -= SPAN;
	if (scale_ < 5)
		scale_ = 5;
	UpdateCoordinate();
}

void CDrawWnd::OnInitSize()
{
	// TODO: �ڴ���������������
	xPos_ = 0;
	yPos_ = 0;
	scale_ = 100;
	rotation_ = 0.0f;
	UpdateCoordinate();

	if (cb_)
		cb_->OnResetSize(width_, height_);
}
