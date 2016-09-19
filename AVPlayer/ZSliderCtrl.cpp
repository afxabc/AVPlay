// ZSliderCtrl.cpp : 实现文件
//

#include "stdafx.h"
#include "AVPlayer.h"
#include "ZSliderCtrl.h"


// ZSliderCtrl

IMPLEMENT_DYNAMIC(ZSliderCtrl, CSliderCtrl)

ZSliderCtrl::ZSliderCtrl()
{
	memDC_.m_hDC = NULL;
	memBmp_.m_hObject = NULL;
}

ZSliderCtrl::~ZSliderCtrl()
{
	DestroyDC();
}

void ZSliderCtrl::SetPos(int pos)
{
	pos = (pos < GetRangeMin()) ? GetRangeMin() : pos;
	pos = (pos > GetRangeMax()) ? GetRangeMax() : pos;
	CSliderCtrl::SetPos(pos);
	drawPos();
}

BEGIN_MESSAGE_MAP(ZSliderCtrl, CSliderCtrl)
	ON_WM_PAINT()
	ON_NOTIFY_REFLECT(NM_THEMECHANGED, &ZSliderCtrl::OnNMThemeChanged)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CREATE()
END_MESSAGE_MAP()

// ZSliderCtrl 消息处理程序

void ZSliderCtrl::ResetDC()
{
	DestroyDC();

	colorBk_ = GetSysColor(COLOR_3DFACE);
	colorPen_ = RGB(64, 64, 64);// GetSysColor(COLOR_3DSHADOW);

	RECT r;
	this->GetClientRect(&r);
	width_ = r.right;
	height_ = r.bottom;

	line_ = width_ - 2 * SPAN;

	CClientDC dc(this);

	memBkDC_.CreateCompatibleDC(&dc);
	memBkBmp_.CreateCompatibleBitmap(&dc, width_, height_);
	memBkDC_.SelectObject(&memBkBmp_);
	memBkDC_.FillSolidRect(&r, colorBk_);
	memBkDC_.Draw3dRect(SPAN, height_ / 2 - 1, line_, 3, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHIGHLIGHT));

	memDC_.CreateCompatibleDC(&dc);
	memBmp_.CreateCompatibleBitmap(&dc, width_, height_);
	memDC_.SelectObject(&memBmp_);

	memPen_.CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
	memDC_.SelectObject(&memPen_);
	memPenSelect_.CreatePen(PS_SOLID, 1, RGB(64, 64, 64));

	memBrush_.CreateSolidBrush(GetSysColor(COLOR_3DFACE));
	memDC_.SelectObject(&memBrush_);
	memBrushSelect_.CreateSolidBrush(GetSysColor(COLOR_3DHIGHLIGHT));

	drawPos();
}

void ZSliderCtrl::DestroyDC()
{
	if (isValidDC())
	{
		memDC_.DeleteDC();
		memDC_.m_hDC = NULL;

		memBmp_.DeleteObject();
		memBkDC_.DeleteDC();
		memBkBmp_.DeleteObject();
		memPen_.DeleteObject();
		memPenSelect_.DeleteObject();
		memBrush_.DeleteObject();
		memBrushSelect_.DeleteObject();
	}
}

void ZSliderCtrl::drawPos()
{
	if (!isValidDC())
		return;

	memDC_.BitBlt(0, 0, width_, height_, &memBkDC_, 0, 0, SRCCOPY);

	range_ = GetRangeMax() - GetRangeMin();
	if (range_ > 0)
	{
		float pos = GetPos() - GetRangeMin();
		int x = line_*pos / range_+SPAN;
		int y = height_/2-1;
		int w = 4;
		int h = height_ / 2 - 2;
		int r = 5;
	//	memDC_.Ellipse(x - r, y - r, x + r, y + r);
	/*
		CPoint pts[5];
		pts[0] = { x - w * 2,	y - w };
		pts[1] = { x - w,		y - w };
		pts[2] = { x,			y };
		pts[3] = { x - w,		y + w };
		pts[4] = { x - w * 2,	y + w };
		memDC_.Polygon(pts, 5);
	*/
	//	memDC_.Rectangle(x-w, y-h, x+w, y+h);
		memDC_.RoundRect(x - w, y - h, x + w, y + h, r, r);
	}
	Invalidate(FALSE);
}

BOOL ZSliderCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此添加专用代码和/或调用基类
	cs.style |= BS_OWNERDRAW;
	return CSliderCtrl::PreCreateWindow(cs);
}

void ZSliderCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CSliderCtrl::OnPaint()
	if (isValidDC())
		dc.BitBlt(0, 0, width_, height_, &memDC_, 0, 0, SRCCOPY);
}

void ZSliderCtrl::OnNMThemeChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	// 该功能要求使用 Windows XP 或更高版本。
	// 符号 _WIN32_WINNT 必须 >= 0x0501。
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	ResetDC();
}

void ZSliderCtrl::OnSize(UINT nType, int cx, int cy)
{
	CSliderCtrl::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	ResetDC();
}

void ZSliderCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	ReleaseCapture();
	tooltip_.Activate(FALSE);
	memDC_.SelectObject(&memPen_);
	memDC_.SelectObject(&memBrush_);
	drawPos();
	CSliderCtrl::OnLButtonUp(nFlags, point);
}

void ZSliderCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (GetCapture() != this)
	{
		SetCapture();
		memDC_.SelectObject(&memPenSelect_);
		memDC_.SelectObject(&memBrushSelect_);
		if (line_ > 0 && range_ > 0)
		{
			DWORD pos = (point.x - SPAN)*range_ / line_;
			SetPos(pos);
			AfxGetApp()->GetMainWnd()->PostMessage(WM_SLIDER_CHANGED, pos, (LPARAM)this);
		}
		tooltip_.Activate(TRUE);
	}
	CSliderCtrl::OnLButtonDown(nFlags, point);
}

void ZSliderCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (GetCapture() == this)
	{
		if (line_ > 0 && range_ > 0)
		{
			int pos = (point.x - SPAN)*range_ / line_;
			SetPos(pos);
			AfxGetApp()->GetMainWnd()->PostMessage(WM_SLIDER_CHANGED, pos, (LPARAM)this);
		}
	}
	CSliderCtrl::OnMouseMove(nFlags, point);
}


int ZSliderCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CSliderCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	tooltip_.Create(this);
	tooltip_.SetDelayTime(0);
	this->SetToolTips(&tooltip_);

	return 0;
}
