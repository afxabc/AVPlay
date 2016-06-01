// ChildView.h : CChildView 类的接口
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

// CChildView 窗口
#define LOAD_BGRA 1
class CChildView : public CWnd
{
// 构造
public:
	CChildView();
	virtual ~CChildView();

	void setCallback(IChildViewCallback* cb)
	{
		cb_ = cb;
	}
	void ResetRect();
	void DrawFrame(const FrameData& f);

// 重写
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	void ResetSurface(int width, int height);
	void Cleanup();

	// 生成的消息映射函数
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

