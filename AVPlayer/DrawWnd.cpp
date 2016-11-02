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
	, isFullScreen_(false)
	, isMoved_(false)
	, wndFitFrm_(true)
	, frmFitWnd_(false)
	, tickCursor_(0)
	, hideCursor_(0)
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
	ON_COMMAND(IDC_WND_FIT_FRM, &CDrawWnd::OnWndFitFrm)
	ON_UPDATE_COMMAND_UI(IDC_WND_FIT_FRM, &CDrawWnd::OnUpdateWndFitFrm)
	ON_WM_MBUTTONUP()
	ON_WM_CLOSE()
	ON_COMMAND(IDC_FRM_FIT_WND, &CDrawWnd::OnFrmFitWnd)
	ON_UPDATE_COMMAND_UI(IDC_FRM_FIT_WND, &CDrawWnd::OnUpdateFrmFitWnd)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CDrawWnd 消息处理程序

int CDrawWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码

	OnDrawWndHandle(IDC_SHOW_D3DSPIRIT);

	parent_ = (CFrameWnd*)GetParent();

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
	isMoved_ = false;
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

		if (!isMoved_)
			parent_->PostMessage(WM_COMMAND, ID_FILE_PAUSE);
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
		isMoved_ = true;
	}
	else if (isFullScreen_)
	{
		int full_y = GetSystemMetrics(SM_CYSCREEN);
		if (cb_)
			cb_->OnShowToolbar(point.y > full_y - 100);

		if (point.y > full_y - 100)
			tickCursor_ = TICK_CURSOR_MAX + 1;
		else tickCursor_ = 0;

		showCursor();
	}
}

void CDrawWnd::OnRotateAngle()
{
	// TODO: 在此添加命令处理程序代码
	rotation_ = (ROTATIONTYPE)((rotation_ + 1) % ROTATION_N);
	
	if (wndFitFrm_ && !isFullScreen_)
		OnWindowFit();
	else if (frmFitWnd_)
		OnFrameFit();
	else UpdateCoordinate(TRUE);
}

void CDrawWnd::OnZoomIn()
{
	// TODO: 在此添加命令处理程序代码
	int SPAN = (scale_>=250) ? 50 :((scale_<100) ? 5 : 10);
	if (scale_ < 500)
		scale_ += SPAN;

	if (wndFitFrm_ && !isFullScreen_)
		OnWindowFit();
	else UpdateCoordinate(TRUE);
}

void CDrawWnd::OnZoomOut()
{
	// TODO: 在此添加命令处理程序代码
	int SPAN = (scale_>250) ? 50 : ((scale_<100) ? 5 : 10);
	if (scale_ > 20)
		scale_ -= SPAN;

	if (wndFitFrm_ && !isFullScreen_)
		OnWindowFit();
	else UpdateCoordinate(TRUE);
}

void CDrawWnd::OnInitSize()
{
	// TODO: 在此添加命令处理程序代码

	if (isFullScreen_)
	{
		KillTimer(1);
		isFullScreen_ = false;
		showCursor();
		if (cb_)
			cb_->OnResetSizeFullScreen(false);
	}

	scale_ = 100;
//	rotation_ = ROTATION_0;
//	UpdateCoordinate(TRUE);

	OnWindowFit();
}

void CDrawWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnLButtonDblClk(nFlags, point);

	int full_x = GetSystemMetrics(SM_CXSCREEN);
	int full_y = GetSystemMetrics(SM_CYSCREEN);
	RECT r;
	this->GetClientRect(&r);

	if (!isMoved_)
		parent_->PostMessage(WM_COMMAND, ID_FILE_PAUSE);

	if (xPos_ != 0 || yPos_ != 0)
	{
		xPos_ = 0;
		yPos_ = 0;
		UpdateCoordinate(TRUE);
	}
	else if (isFullScreen_ || scale_!=100)
		OnInitSize();
	else OnFullScreen();
}

void CDrawWnd::OnMButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CWnd::OnMButtonUp(nFlags, point);
	OnWindowFit();
}

void CDrawWnd::OnWindowFit()
{
	// TODO: 在此添加命令处理程序代码3
	if (isFullScreen_)
		return;

	xPos_ = 0;
	yPos_ = 0;
	UpdateCoordinate(TRUE);

	int width = (rotation_ == ROTATION_0 || rotation_ == ROTATION_180) ? width_ : height_;
	int height = (rotation_ == ROTATION_0 || rotation_ == ROTATION_180) ? height_ : width_;
	if (cb_)
		cb_->OnResetSize(width*scale_/100, height*scale_/100);
}

void CDrawWnd::OnWndFitFrm()
{
	wndFitFrm_ = !wndFitFrm_;
	if (wndFitFrm_)
	{
		frmFitWnd_ = false;
		OnWindowFit();
	}
}

void CDrawWnd::OnUpdateWndFitFrm(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->SetCheck(wndFitFrm_);
}

void CDrawWnd::OnFrameFit()
{
	// TODO: 在此添加命令处理程序代码3
	int width = (rotation_ == ROTATION_0 || rotation_ == ROTATION_180) ? width_ : height_;
	int height = (rotation_ == ROTATION_0 || rotation_ == ROTATION_180) ? height_ : width_;

	if (width <= 0 || height <= 0)
		return;

	float scalex = (float)WIDTH_ * 100 / width;
	float scaley = (float)HEIGHT_ * 100 / height;

	scale_ = (scalex > scaley) ? scaley : scalex;
	if (abs(scale_ - 100) <= 2)
		scale_ = 100;

	UpdateCoordinate(TRUE);
}

void CDrawWnd::OnFrmFitWnd()
{
	// TODO: 在此添加命令处理程序代码
	frmFitWnd_ = !frmFitWnd_;
	if (frmFitWnd_)
	{
		wndFitFrm_ = false;
		OnFrameFit();
	}
}

void CDrawWnd::OnUpdateFrmFitWnd(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->SetCheck(frmFitWnd_);
}

void CDrawWnd::OnFullScreen()
{
	xPos_ = 0;
	yPos_ = 0;

	isFullScreen_ = true;
	tickCursor_ = TICK_CURSOR_MAX - 1;
	if (cb_)
		cb_->OnResetSizeFullScreen(true);

	UpdateCoordinate(TRUE);
	this->SetFocus();
	
	SetTimer(1, 1000, NULL);
}

void CDrawWnd::OnResetDevice()
{
	if (pHandle_ == NULL)
		return;

	if (frmFitWnd_)
		OnFrameFit();
	else UpdateCoordinate(TRUE);
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

void CDrawWnd::procKeyDown(UINT nChar)
{
	switch (nChar)
	{
	case VK_SPACE:
		if (!keyDown_)
			parent_->PostMessage(WM_COMMAND, ID_FILE_PAUSE);
		break;
	case VK_RIGHT:
		parent_->PostMessage(WM_COMMAND, IDC_SEEK_FORWARD);
		break;
	case VK_LEFT:
		parent_->PostMessage(WM_COMMAND, IDC_SEEK_BACKWARD);
		break;
	case VK_UP:
		parent_->PostMessage(WM_COMMAND, ID_VOLUME_UP);
		break;
	case VK_DOWN:
		parent_->PostMessage(WM_COMMAND, ID_VOLUME_DOWN);
		break;
	}

	keyDown_ = true;
}

void CDrawWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	procKeyDown(nChar);
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CDrawWnd::procKeyUp(UINT nChar)
{
	keyDown_ = false;
}

void CDrawWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	procKeyUp(nChar);
	CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CDrawWnd::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (isFullScreen_)
		return;

	CWnd::OnClose();
}

void CDrawWnd::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (isFullScreen_ && tickCursor_ < TICK_CURSOR_MAX)
	{
		tickCursor_++;
		if (tickCursor_ == TICK_CURSOR_MAX)
		{
			hideCursor();
			LOGW("Hide cursor");
		}
	}
	CWnd::OnTimer(nIDEvent);
}
