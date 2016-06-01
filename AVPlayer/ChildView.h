// ChildView.h : CChildView ��Ľӿ�
//


#pragma once

class IChildViewCallback
{
public:
	virtual void OnResetSize(int width, int height) = 0;
};

/////////////////////////////////////////////////////

#include "FrameData.h"
#include <ddraw.h>

// CChildView ����
#define LOAD_BGRA 1
class CChildView : public CWnd
{
// ����
public:
	CChildView();
	virtual ~CChildView();

	void setCallback(IChildViewCallback* cb)
	{
		cb_ = cb;
	}
	void ResetRect();
	void DrawFrame(const FrameData& f);

// ��д
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	void ResetSurface(int width, int height);
	void Cleanup();

	// ���ɵ���Ϣӳ�亯��
protected:

	IChildViewCallback* cb_;
	RECT rect_;

	//directx
	int width_;
	int height_;
	LPDIRECTDRAW lpDirectDraw_;	LPDIRECTDRAWSURFACE lpSurface_, lpBkSurface_;	LPDIRECTDRAWCLIPPER lpClipper_;
public:
	DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
};

