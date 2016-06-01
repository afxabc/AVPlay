
// MainFrm.h : CMainFrame 类的接口
//

#pragma once

//#include "ChildView.h"
//#include "DrawWnd.h"
#include "DrawWndD3d.h"
#include "Player.h"
#include "base\queue.h"

class CMainFrame : public CFrameWnd, IChildViewCallback
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// 特性
public:
	virtual void OnResetSize(int width, int height);
	virtual void ReportParams(float x, float y, float scale, float rotate, int width, int height, const RECT& r);

// 操作
public:

// 重写
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// 实现
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void onFrameData(FrameData frm); 
	void print(Log::LEVEL level, const char * sformat);

protected:  // 控件条嵌入成员
	CToolBar          m_wndToolBar;
	CStatusBar        m_wndStatusBar;
//	CChildView    m_wndView;
//	CDrawWnd    m_wndView;
	CDrawWndD3d    m_wndView;
	Player player_;
	CString fipath_;
	Queue<FrameData> frms_;

// 生成的消息映射函数
protected:
	bool checkFilePath();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnFileOpen();
	afx_msg void OnFilePlay();
	afx_msg void OnDrawFrame();
	afx_msg void OnUpdateFilePlay(CCmdUI *pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
};


