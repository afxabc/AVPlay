#pragma once


class IDrawWndCallback
{
public:
	virtual void OnResetSize(int width, int height) = 0;
	virtual void ReportParams(float scale, float rotate, POINT pos, SIZE szFrm, SIZE szWnd) = 0;
};

#include "FrameData.h"
#include "IDrawWndHandle.h"

// CDrawWnd

class CDrawWnd : public CWnd
{
	DECLARE_DYNAMIC(CDrawWnd)

public:
	CDrawWnd();
	virtual ~CDrawWnd();

	void setCallback(IDrawWndCallback* cb)
	{
		cb_ = cb;
	}

	void DrawFrame(const FrameData& f);

protected:
	void UpdateCoordinate(BOOL render = FALSE)
	{
		pHandle_->UpdateCoordinate(scale_ / 100.0f, rotation_, CPoint(xPos_, yPos_), CSize(width_, height_), CSize(WIDTH_, HEIGHT_));

		if (cb_)
			cb_->ReportParams(scale_ / 100.0f, rotation_, CPoint(xPos_, yPos_), CSize(width_, height_), CSize(WIDTH_, HEIGHT_));

		if (render)
			pHandle_->Render();
	}

protected:
	IDrawWndCallback* cb_;

	float width_;
	float height_;

	int WIDTH_;
	int HEIGHT_;

	int scale_;	//scale_/100
	int xPos_;
	int yPos_;
	POINT posMove_;
	float rotation_;

	IDrawWndHandle* pHandle_;

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRotateAngle();
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();

public:
	afx_msg void OnInitSize();
};


