#pragma once

#include "DrawWndHandle.h"


class IDrawWndCallback
{
public:
	virtual void OnResetSize(int width, int height) = 0;
	virtual void OnResetSizeFullScreen(bool isFull) = 0;
	virtual void OnShowToolbar(BOOL show) = 0;
	virtual void ReportParams(int scale, ROTATIONTYPE rotate, POINT pos, SIZE szFrm, SIZE szWnd) = 0;
};

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
	void ResetDrawWndHandle();

	void ReDraw()
	{
		if (pHandle_)
			pHandle_->Render();
	}

	inline bool SaveFrame(const char* fipath)
	{
		return frmBak_.toFile(fipath);
	}

	const FrameData& frame()
	{
		return frmBak_;
	}

	bool isFullScreen()
	{
		return isFullScreen_;
	}

	afx_msg void OnInitSize();

protected:
	void UpdateCoordinate(BOOL render = FALSE)
	{
		if (pHandle_)
		{
			pHandle_->UpdateCoordinate(scale_ / 100.0f, rotation_, CPoint(xPos_, yPos_), frmBak_, this->GetSafeHwnd());
			if (render)
				pHandle_->Render();
		}

		if (cb_)
			cb_->ReportParams(scale_, rotation_, CPoint(xPos_, yPos_), CSize(width_, height_), CSize(WIDTH_, HEIGHT_));
	}

	bool checkForEdge();

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
	ROTATIONTYPE rotation_;

	IDrawWndHandle* pHandle_;
	UINT idHandle_;

	FrameData frmBak_;
	bool keyDown_;

	CFrameWnd* parent_;
	bool isFullScreen_;

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnWindowFit();
	afx_msg void OnFullScreen();
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRotateAngle();
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnResetDevice();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDrawWndHandle(UINT which);
	afx_msg void OnUpdateShowDDraw(CCmdUI *pCmdUI);
	afx_msg void OnUpdateShowSurface(CCmdUI *pCmdUI);
	afx_msg void OnUpdateShowSpirit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateShowVertex(CCmdUI *pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
};


   