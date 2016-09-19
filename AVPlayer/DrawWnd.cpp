// DrawWnd.cpp : 实现文件
//

#include "stdafx.h"
#include "AVPlayer.h"
#include "DrawWnd.h"

#include "DrawWndDDraw.h"
#include "DrawWndSurface.h"
#include "DrawWndSprite.h"
#include "DrawWndVertex.h"

// CDrawWnd

IMPLEMENT_DYNAMIC(CDrawWnd, CWnd)

static const int INIT_WIDTH = 765;
static const int INIT_HEIGHT = 450;
CDrawWnd::CDrawWnd()
	: cb_(NULL)
	, pHandle_(NULL)
	, scale_(100)
	, xPos_(0.0f)
	, yPos_(0.0f)
	, width_(INIT_WIDTH)
	, height_(INIT_HEIGHT)
	, WIDTH_(INIT_WIDTH)
	, HEIGHT_(INIT_HEIGHT)
	, rotation_(ROTATION_0)
	, idHandle_(0)
	, keyDown_(false)
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
/*		UpdateCoordinate();*/
		if (cb_)
			cb_->OnResetSize(width_, height_);
	}

	pHandle_->DrawFrame(f.data_, f.width_, f.height_);

	frmBak_ = f;
}

void CDrawWnd::ResetDrawWndHandle()
{
	if (!pHandle_)
		return;

	if (pHandle_->IsValid())
		pHandle_->Cleanup();

	pHandle_->CreateDevice(GetSafeHwnd());
	UpdateCoordinate(TRUE);
}

bool CDrawWnd::checkForEdge()
{
	return false;
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
	ON_COMMAND(IDC_RESET_DEVICE, &CDrawWnd::OnResetDevice)
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND_RANGE(IDC_SHOW_START, IDC_SHOW_END, &CDrawWnd::OnDrawWndHandle)
	ON_UPDATE_COMMAND_UI(IDC_SHOW_DIRECTDRAW, &CDrawWnd::OnUpdateShowDDraw)
	ON_UPDATE_COMMAND_UI(IDC_SHOW_D3DSURFACE, &CDrawWnd::OnUpdateShowSurface)
	ON_UPDATE_COMMAND_UI(IDC_SHOW_D3DSPIRIT, &CDrawWnd::OnUpdateShowSpirit)
	ON_UPDATE_COMMAND_UI(IDC_SHOW_D3DVERTEX, &CDrawWnd::OnUpdateShowVertex)

	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_COMMAND(IDC_WINDOW_FIT, &CDrawWnd::OnWindowFit)
	ON_WM_MBUTTONUP()
END_MESSAGE_MAP()

// CDrawWnd 消息处理程序

int CDrawWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码

	OnDrawWndHandle(IDC_SHOW_D3DSPIRIT);

	return 0;
}

void CDrawWnd::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	delete pHandle_;
	pHandle_ = NULL;
}

void CDrawWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	WIDTH_ = cx;
	HEIGHT_ = cy;
	
	this->PostMessage(WM_COMMAND, IDC_RESET_DEVICE);
}

BOOL CDrawWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (zDelta > 0)
		OnZoomIn();
	else OnZoomOut();
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CDrawWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnRButtonUp(nFlags, point);
	OnRotateAngle();
}

void CDrawWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnLButtonDown(nFlags, point);
	SetCapture();
	posMove_ = point;
}

void CDrawWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnLButtonUp(nFlags, point);
	if (GetCapture() == this)
	{
		ReleaseCapture();
		xPos_ += (point.x - posMove_.x);
		yPos_ += (point.y - posMove_.y);
		UpdateCoordinate(TRUE);
	}
}

void CDrawWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnMouseMove(nFlags, point);
	if (GetCapture() == this)
	{
		xPos_ += (point.x - posMove_.x);
		yPos_ += (point.y - posMove_.y);
		UpdateCoordinate(TRUE);
		posMove_ = point;
	}
}

void CDrawWnd::OnRotateAngle()
{
	// TODO: 在此添加命令处理程序代码
	rotation_ = (ROTATIONTYPE)((rotation_ + 1) % ROTATION_N);
	UpdateCoordinate(TRUE);
}

void CDrawWnd::OnZoomIn()
{
	// TODO: 在此添加命令处理程序代码
	int SPAN = (scale_>=250) ? 50 :((scale_<100) ? 5 : 10);
	scale_ += SPAN;
	if (scale_ > 500)
		scale_ = 500;
	UpdateCoordinate(TRUE);
}

void CDrawWnd::OnZoomOut()
{
	// TODO: 在此添加命令处理程序代码
	int SPAN = (scale_>250) ? 50 : ((scale_<100) ? 5 : 10);
	scale_ -= SPAN;
	if (scale_ < 10)
		scale_ = 10;
	UpdateCoordinate(TRUE);
}

void CDrawWnd::OnInitSize()
{
	// TODO: 在此添加命令处理程序代码
	xPos_ = 0;
	yPos_ = 0;
	scale_ = 100;
	rotation_ = ROTATION_0;
	UpdateCoordinate(TRUE);

	if (cb_)
		cb_->OnResetSize(width_, height_);
}

void CDrawWnd::OnWindowFit()
{
	// TODO: 在此添加命令处理程序代码

	xPos_ = 0;
	yPos_ = 0;
	UpdateCoordinate(TRUE);

	int width = (rotation_ == ROTATION_0 || rotation_ == ROTATION_180) ? width_ : height_;
	int height = (rotation_ == ROTATION_0 || rotation_ == ROTATION_180) ? height_ : width_;
	if (cb_)
		cb_->OnResetSize(width*scale_/100, height*scale_/100);
}

void CDrawWnd::OnResetDevice()
{
	if (pHandle_ == NULL)
		return;

	UpdateCoordinate(TRUE);
}

void CDrawWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnLButtonDblClk(nFlags, point);

	xPos_ = 0;
	yPos_ = 0;
	scale_ = 100;
	UpdateCoordinate(TRUE);
}

void CDrawWnd::OnDrawWndHandle(UINT which)
{
	if (which == idHandle_)
		return;

	idHandle_ = which;

	if (pHandle_)
	delete pHandle_, pHandle_ = NULL;

	switch (which)
	{
	case IDC_SHOW_DIRECTDRAW:
		pHandle_ = new CDrawWndDDraw(this);
		break;
	case IDC_SHOW_D3DSURFACE:
		pHandle_ = new CDrawWndSurface();
		break;
	case IDC_SHOW_D3DSPIRIT:
		pHandle_ = new CDrawWndSprite();
		break;
	case IDC_SHOW_D3DVERTEX:
		pHandle_ = new CDrawWndVertex();
		break;
	}

	ResetDrawWndHandle();
}

void CDrawWnd::OnUpdateShowDDraw(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(IDC_SHOW_DIRECTDRAW == idHandle_);
}

void CDrawWnd::OnUpdateShowSurface(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(IDC_SHOW_D3DSURFACE == idHandle_);
}

void CDrawWnd::OnUpdateShowSpirit(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(IDC_SHOW_D3DSPIRIT == idHandle_);
}

void CDrawWnd::OnUpdateShowVertex(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(IDC_SHOW_D3DVERTEX == idHandle_);
}

void CDrawWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nChar == VK_SPACE && !keyDown_)
	{
		keyDown_ = true;
		GetParent()->PostMessage(WM_COMMAND, ID_FILE_PAUSE);
	}
	else if (nChar == VK_RIGHT)
		GetParent()->PostMessage(WM_COMMAND, IDC_SEEK_FORWARD);
	else if (nChar == VK_LEFT)
		GetParent()->PostMessage(WM_COMMAND, IDC_SEEK_BACKWARD);

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CDrawWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nChar == ' ')
		keyDown_ = false;
	CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}


void CDrawWnd::OnMButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	OnWindowFit();
	CWnd::OnMButtonUp(nFlags, point);
}
