#pragma once


// ZSliderCtrl
#define WM_SLIDER_CHANGED WM_USER+1010
#define WM_SLIDER_SELECTED WM_USER+1011

class ZSliderCtrl : public CSliderCtrl
{
	DECLARE_DYNAMIC(ZSliderCtrl)

public:
	ZSliderCtrl();
	virtual ~ZSliderCtrl();
	void SetPos(int pos);

protected:
	void ResetDC();
	void DestroyDC();
	void drawPos();

	bool isValidDC()
	{
		return (memDC_.m_hDC != NULL);
	}

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
	CToolTipCtrl tooltip_;

	static const int SPAN = 10;

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};


