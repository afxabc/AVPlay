// ZSliderCtrl.cpp : ʵ���ļ�
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
	, transparentBall_(TRANSPARENT_NORMAL)
{
	memDC_.m_hDC = NULL;
	memDCBall_.m_hDC = NULL;

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
	DestroyBall();
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
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// ZSliderCtrl ��Ϣ�������

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

	memDC_.SelectObject(&font_);

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
	}

}

void ZSliderCtrl::DestroyBall()
{
	if (isValidBall())
	{
		memDCBall_.DeleteDC();
		memDCBall_.m_hDC = NULL;
		memBmpBall_.DeleteObject();
	}
}

void ZSliderCtrl::drawBk()
{
	if (!isValidDC())
		return;

	RECT r;
	this->GetClientRect(&r);

	memBkDC_.FillSolidRect(&r, colorBk_);

	static const COLORREF clrDark = RGB(96, 96, 96);
	static const COLORREF clrLight = RGB(160, 160, 160);

	static const int sliderWidth = 5;
	if (isHorz_)
		memBkDC_.Draw3dRect(SPAN, height_ / 2 - sliderWidth/2 - 1, line_, sliderWidth, clrLight, clrLight);
	else memBkDC_.Draw3dRect(width_ / 2 - sliderWidth/2 - 1, SPAN, sliderWidth, line_, clrLight, clrLight);

	if (getSelectRange() > 0)
	{
		CPen penSelect;
		penSelect.CreatePen(PS_SOLID, 3, clrLight);
		CPen penLine;
		penLine.CreatePen(PS_SOLID, 1, clrDark);

		CPoint ptMin = pos2Point(posSelectMin_, -1);
		CPoint ptMax = pos2Point(posSelectMax_, -1);

		memBkDC_.SelectObject(&penSelect);
		memBkDC_.MoveTo(ptMin);
		memBkDC_.LineTo(ptMax);

		memBkDC_.SelectObject(&penLine);
		int span = 2;
		int th = 3;
		if (isHorz_)
		{
			memBkDC_.MoveTo(ptMin.x, span);
			memBkDC_.LineTo(ptMin.x, height_- span - 1);
			//������
			memBkDC_.MoveTo(ptMin.x- th, height_/2- th -1);
			memBkDC_.LineTo(ptMin.x, height_/2 -1);
			memBkDC_.LineTo(ptMin.x- th, height_/2+ th -1);
			memBkDC_.LineTo(ptMin.x- th, height_/2- th -1);

			memBkDC_.MoveTo(ptMax.x, span);
			memBkDC_.LineTo(ptMax.x, height_- span - 1);
			//������
			memBkDC_.MoveTo(ptMax.x+ th, height_/2- th -1);
			memBkDC_.LineTo(ptMax.x, height_/2 -1);
			memBkDC_.LineTo(ptMax.x+ th, height_/2+ th -1);
			memBkDC_.LineTo(ptMax.x+ th, height_/2- th -1);
		}
		else
		{
			memBkDC_.MoveTo(span, ptMin.y);
			memBkDC_.LineTo(width_- span - 1, ptMin.y);
			//������
			memBkDC_.MoveTo(width_/2+th-1, ptMin.y+th);
			memBkDC_.LineTo(width_/2-1, ptMin.y);
			memBkDC_.LineTo(width_/2-th-1, ptMin.y+th);
			memBkDC_.LineTo(width_/2+th-1, ptMin.y+th);

			memBkDC_.MoveTo(span, ptMax.y);
			memBkDC_.LineTo(width_- span - 1, ptMax.y);
			//������
			memBkDC_.MoveTo(width_ / 2 + th-1, ptMax.y-th);
			memBkDC_.LineTo(width_/2-1, ptMax.y);
			memBkDC_.LineTo(width_/2-th-1, ptMax.y-th);
			memBkDC_.LineTo(width_/2+th-1, ptMax.y-th);
		}
	}

	drawPos();
}

void ZSliderCtrl::drawPos()
{
	if (!isValidDC())
		return;

	memDC_.BitBlt(0, 0, width_, height_, &memBkDC_, 0, 0, SRCCOPY);

	if (!isValidBall() && !ResetBall())
		return;

	float pos = GetPos() - GetRangeMin();
	int x = isHorz_?(line_*pos / range_+SPAN):(width_/2-1);
	int y = isHorz_?(height_/2-1):(height_ - line_*pos / range_-SPAN);

//	memDC_.BitBlt(x - widthBall_ / 2, y - heightBall_ / 2, widthBall_, heightBall_, &memDCBall_, 0, 0, SRCCOPY);

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = transparentBall_;  
	bf.AlphaFormat = 0;
	memDC_.AlphaBlend(x - widthBall_ / 2, y - heightBall_ / 2, widthBall_, heightBall_, 
		&memDCBall_, 0, 0, widthBall_, heightBall_, bf);    

	Invalidate(FALSE);
}

bool ZSliderCtrl::ResetBall(bool isPush)
{
	DestroyBall();

	if (range_ <= 0)
		return false;

	float pos = GetPos() - GetRangeMin();

	static const int W = 11;
	static const int H_SPAN = 2;

	widthBall_ = isHorz_ ? W : (width_ - H_SPAN*2 + 1);
	heightBall_ = isHorz_ ? (height_ - H_SPAN*2 + 1) : W;

	CClientDC dc(this);

	memDCBall_.CreateCompatibleDC(&dc);
	memBmpBall_.CreateCompatibleBitmap(&dc, widthBall_, heightBall_);
	memDCBall_.SelectObject(&memBmpBall_);

	static const COLORREF clrPen = RGB(64, 64, 64);
	static const COLORREF clrPenPush = RGB(0, 64, 128);
	CPen penLine;
	CPen penRect;
	penLine.CreatePen(PS_SOLID, 1, isPush ? clrPenPush:clrPen);
	penRect.CreatePen(PS_SOLID, 1, isPush ? clrPenPush : clrPen);

	static const COLORREF clrBallBk = GetSysColor(COLOR_3DFACE);
	static const COLORREF clrBallBkPush = RGB(128, 192, 255); //GetSysColor(COLOR_3DFACE);
	CBrush brushRect;
	brushRect.CreateSolidBrush(isPush? clrBallBkPush:clrBallBk);

	memDCBall_.SelectObject(&penRect);
	memDCBall_.SelectObject(&brushRect);
	memDCBall_.Rectangle(0, 0, widthBall_, heightBall_);

	memDCBall_.SelectObject(&penLine);
	if (isHorz_)
	{
		memDCBall_.MoveTo(widthBall_ /2, heightBall_/4);
		memDCBall_.LineTo(widthBall_ /2, heightBall_ - heightBall_ / 4);
	}
	else
	{
		memDCBall_.MoveTo(widthBall_/4, heightBall_ /2);
		memDCBall_.LineTo(widthBall_ - widthBall_ / 4, heightBall_ /2);
	}

	return true;
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
	// TODO: �ڴ����ר�ô����/����û���
//	cs.style |= BS_OWNERDRAW;

//	if ((TBS_VERT & cs.style) != 0)
//		isHorz_ = FALSE;

	return CSliderCtrl::PreCreateWindow(cs);
}

void ZSliderCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: �ڴ˴������Ϣ����������
					   // ��Ϊ��ͼ��Ϣ���� CSliderCtrl::OnPaint()
	if (!isValidDC())
		ResetMDC();

	dc.BitBlt(0, 0, width_, height_, &memDC_, 0, 0, SRCCOPY);
}

void ZSliderCtrl::OnNMThemeChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	// �ù���Ҫ��ʹ�� Windows XP ����߰汾��
	// ���� _WIN32_WINNT ���� >= 0x0501��
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	ResetMDC();
}

void ZSliderCtrl::OnSize(UINT nType, int cx, int cy)
{
	CSliderCtrl::OnSize(nType, cx, cy);

	// TODO: �ڴ˴������Ϣ����������
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

CPoint ZSliderCtrl::pos2Point(int pos, int df)
{
	CPoint point;

	pos = (pos < GetRangeMin()) ? GetRangeMin() : pos;
	pos = (pos > GetRangeMax()) ? GetRangeMax() : pos;

	point.x = width_ / 2 + df;
	point.y = height_ / 2 + df;

	if (range_ <= 0)
		return point;

	if (isHorz_)
		point.x = pos*line_ / range_ + SPAN;	//pos = (point.x - SPAN)*range_ / line_;
	else point.y = height_ - (pos*line_ / range_ + SPAN);	//pos = ((height_ - point.y) - SPAN)*range_ / line_;

	return point;
}

void ZSliderCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CSliderCtrl::OnLButtonUp(nFlags, point);

//	if (GetCapture() == this)
	{
		ReleaseCapture();
		transparentBall_ = TRANSPARENT_NORMAL;//��͸��(0-ff,͸���ȴ�ȫ͸������͸��) 
		ResetBall();
		int pos = point2Pos(point);
		pos = SetPos(pos);
	}
	
}

void ZSliderCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (GetCapture() != this)
	{
		SetCapture();
		transparentBall_ = TRANSPARENT_PUSH;//��͸��(0-ff,͸���ȴ�ȫ͸������͸��) 
		ResetBall(true);

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
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
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

void ZSliderCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CSliderCtrl::OnRButtonUp(nFlags, point);

	posSelectMax_ = posSelectMin_ = 0;
	drawBk();
}
