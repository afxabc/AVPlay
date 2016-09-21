#pragma once


// ZSliderCtrl
#define WM_SLIDER_CHANGED WM_USER+1010
#define WM_SLIDER_SELECTED WM_SLIDER_CHANGED+1
#define WM_SLIDER_HOVER WM_SLIDER_CHANGED+2

class ZSliderCtrl : public CSliderCtrl
{
	DECLARE_DYNAMIC(ZSliderCtrl)

public:
	ZSliderCtrl(BOOL isHorz = TRUE);
	virtual ~ZSliderCtrl();

	int SetPos(int pos);
	int GetPos()
	{
		return pos_;
	}

	CWnd* setWndCallback(CWnd* pwnd)
	{
		CWnd* ret = pwndCallback_;
		pwndCallback_ = pwnd;
		return ret;
	}

protected:
	void ResetDC();
	void DestroyDC();
	void drawPos();

	bool isValidDC()
	{
		return (memDC_.m_hDC != NULL);
	}

	void callbackMessage(UINT msg, WPARAM w);

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
	afx_msg void OnPaint();
	afx_msg void OnNMThemeChanged(NMHDR *pNMHDR, LRESULT *pResult);

protected:
	COLORREF colorBk_;
	COLORREF colorPen_;
	int width_;
	int height_;
	float line_;
	float range_;
	CPen memPen_;
	CPen memPenSelect_;
	CBrush memBrush_;
	CBrush memBrushSelect_;
	CDC memDC_;
	CBitmap memBmp_;
	CDC memBkDC_;
	CBitmap memBkBmp_;
	CFont font_;
	int pos_;
	BOOL isHorz_;
	static const int SPAN = 10;

	CWnd* pwndCallback_;

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};


