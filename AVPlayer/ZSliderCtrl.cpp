// ZSliderCtrl.cpp : 实现文件
//

#include "stdafx.h"
#include "AVPlayer.h"
#include "ZSliderCtrl.h"

#include "base/timestamp.h"



// ZSliderCtrl

IMPLEMENT_DYNAMIC(ZSliderCtrl, CSliderCtrl)

ZSliderCtrl::ZSliderCtrl(BOOL isHorz)
	: isHorz_(isHorz)
	, pwndCallback_(NULL)
	, pos_(0)
	, posSelectMin_(0)
	, posSelectMax_(0)
{
	memDC_.m_hDC = NULL;
	memBmp_.m_hObject = NULL;

	font_.CreateFont(
		12,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		FW_NORMAL,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		DEFAULT_CHARSET,             // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		"Arial");

}

ZSliderCtrl::~ZSliderCtrl()
{
	DestroyDC();
}

int ZSliderCtrl::SetPos(int pos)
{
	pos_ = pos;
	pos_ = (pos < GetRangeMin()) ? GetRangeMin() : pos;
	pos_ = (pos > GetRangeMax()) ? GetRangeMax() : pos;
	CSliderCtrl::SetPos(pos_);
//	LOGW("SetPos pos=%d GetPos()=%d", pos, GetPos());
	drawPos();

	return pos_;
}

BEGIN_MESSAGE_MAP(ZSliderCtrl, CSliderCtrl)
	ON_WM_PAINT()
	ON_NOTIFY_REFLECT(NM_THEMECHANGED, &ZSliderCtrl::OnNMThemeChanged)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

// ZSliderCtrl 消息处理程序

void ZSliderCtrl::ResetMDC()
{
	DestroyDC();

	colorBk_ = GetSysColor(COLOR_3DFACE);
	colorPen_ = RGB(64, 64, 64);// GetSysColor(COLOR_3DSHADOW);

	RECT r;
	this->GetClientRect(&r);
	width_ = r.right;
	height_ = r.bottom;

	if (isHorz_)
		line_ = width_ - 2 * SPAN;
	else line_ = height_ - 2 * SPAN;

	CClientDC dc(this);

	memBkDC_.CreateCompatibleDC(&dc);
	memBkBmp_.CreateCompatibleBitmap(&dc, width_, height_);
	memBkDC_.SelectObject(&memBkBmp_);

	memDC_.CreateCompatibleDC(&dc);
	memBmp_.CreateCompatibleBitmap(&dc, width_, height_);
	memDC_.SelectObject(&memBmp_);

	memPen_.CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
	memPenPush_.CreatePen(PS_SOLID, 1, RGB(64, 64, 64));
	memPenSelect_.CreatePen(PS_SOLID, 2, RGB(64, 64, 64));
	memDC_.SelectObject(&memPen_);

	memBrush_.CreateSolidBrush(GetSysColor(COLOR_3DFACE));
	memBrushPush_.CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
	memDC_.SelectObject(&memBrush_);

	memDC_.SelectObject(&font_);

	memBkDC_.SelectObject(&memPenSelect_);

	drawBk();
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
		memPenPush_.DeleteObject();
		memPenSelect_.DeleteObject();
		memBrush_.DeleteObject();
		memBrushPush_.DeleteObject();
	}
}

void ZSliderCtrl::drawBk()
{
	if (!isValidDC())
		return;

	RECT r;
	this->GetClientRect(&r);

	memBkDC_.FillSolidRect(&r, colorBk_);

	if (isHorz_)
		memBkDC_.Draw3dRect(SPAN, height_ / 2 - 1, line_, 3, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHIGHLIGHT));
	else memBkDC_.Draw3dRect(width_ / 2 - 1, SPAN, 3, line_, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHIGHLIGHT));

	if (getSelectRange() > 0)
	{
		CPoint point = pos2Point(posSelectMin_);
		memBkDC_.MoveTo(point);
		point = pos2Point(posSelectMax_);
		memBkDC_.LineTo(point);
	}

	drawPos();
}

void ZSliderCtrl::drawPos()
{
	if (!isValidDC())
		return;

	memDC_.BitBlt(0, 0, width_, height_, &memBkDC_, 0, 0, SRCCOPY);

	range_ = GetRangeMax() - GetRangeMin();
	if (range_ <= 0)
		return;

	float pos = GetPos() - GetRangeMin();
	int r = 5;

	int x = isHorz_?(line_*pos / range_+SPAN):(width_/2-1);
	int y = isHorz_?(height_/2-1):(height_ - line_*pos / range_-SPAN);
	int w = isHorz_?4:(width_ / 2 - 2);
	int h = isHorz_?(height_ / 2 - 2):4;

	memDC_.RoundRect(x - w, y - h, x + w, y + h, r, r);

//	LOGW("drawPos pos=%f", pos);
	Invalidate(FALSE);
}

void ZSliderCtrl::callbackMessage(UINT msg, WPARAM w)
{
	CWnd* pwnd = (pwndCallback_)? pwndCallback_: AfxGetApp()->GetMainWnd();
	if (pwnd == NULL)
		return;
	pwnd->PostMessage(msg, w, (LPARAM)this);
}

BOOL ZSliderCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此添加专用代码和/或调用基类
//	cs.style |= BS_OWNERDRAW;

//	if ((TBS_VERT & cs.style) != 0)
//		isHorz_ = FALSE;

	return CSliderCtrl::PreCreateWindow(cs);
}

void ZSliderCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 在此处添加消息处理程序代码
					   // 不为绘图消息调用 CSliderCtrl::OnPaint()
	if (!isValidDC())
		ResetMDC();

	dc.BitBlt(0, 0, width_, height_, &memDC_, 0, 0, SRCCOPY);
}

void ZSliderCtrl::OnNMThemeChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	// 该功能要求使用 Windows XP 或更高版本。
	// 符号 _WIN32_WINNT 必须 >= 0x0501。
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	ResetMDC();
}

void ZSliderCtrl::OnSize(UINT nType, int cx, int cy)
{
	CSliderCtrl::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	ResetMDC();
}

int ZSliderCtrl::point2Pos(const CPoint& point)
{
	int pos;

	if (isHorz_)
		pos = (point.x - SPAN)*range_ / line_;
	else pos = ((height_ - point.y) - SPAN)*range_ / line_;

	pos = (pos < GetRangeMin()) ? GetRangeMin() : pos;
	pos = (pos > GetRangeMax()) ? GetRangeMax() : pos;

	return pos;
}

CPoint ZSliderCtrl::pos2Point(int pos)
{
	CPoint point;

	pos = (pos < GetRangeMin()) ? GetRangeMin() : pos;
	pos = (pos > GetRangeMax()) ? GetRangeMax() : pos;

	point.x = width_ / 2;
	point.y = height_ / 2;

	if (range_ <= 0)
		return point;

	if (isHorz_)
		point.x = pos*line_ / range_ + SPAN;	//pos = (point.x - SPAN)*range_ / line_;
	else point.y = height_ - (pos*line_ / range_ + SPAN);	//pos = ((height_ - point.y) - SPAN)*range_ / line_;

	return point;
}

void ZSliderCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CSliderCtrl::OnLButtonUp(nFlags, point);

//	if (GetCapture() == this)
	{
		ReleaseCapture();
		memDC_.SelectObject(&memPen_);
		memDC_.SelectObject(&memBrush_);
//		LOGW("OnLButtonUp pos=%d", GetPos());

		int pos = point2Pos(point);
		pos = SetPos(pos);
	}
	
}

void ZSliderCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (GetCapture() != this)
	{
		SetCapture();
		memDC_.SelectObject(&memPenPush_);
		memDC_.SelectObject(&memBrushPush_);

		if (line_ > 0 && range_ > 0)
		{
			int pos = point2Pos(point);
			pos = SetPos(pos);
			callbackMessage(WM_SLIDER_SELECTED, pos);
			callbackMessage(WM_SLIDER_CHANGED, pos);
		}
	}
	CSliderCtrl::OnLButtonDown(nFlags, point);
}

void ZSliderCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CSliderCtrl::OnMouseMove(nFlags, point);

	if (line_ > 0 && range_ > 0)
	{
		int pos = point2Pos(point);
		if (GetCapture() == this)
		{
			pos = SetPos(pos);
			callbackMessage(WM_SLIDER_CHANGED, pos);

//			LOGW("OnMouseMove pos=%d", pos);
		}
		else callbackMessage(WM_SLIDER_HOVER, pos);
	}
}
// F:\AVPlay\AVPlayer\ZSliderCtrl.cpp : 实现文件
//
