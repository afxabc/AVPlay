
// MainFrm.h : CMainFrame 类的接口
//

#pragma once

#include "DrawWnd.h"
#include "Player.h"
#include "base\queue.h"

class CMainFrame : public CFrameWnd, IDrawWndCallback
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// 特性
public:
	virtual void OnResetSize(int width, int height);
	virtual void ReportParams(float scale, float rotate, POINT pos, SIZE szFrm, SIZE szWnd);

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
	CSliderCtrl		  m_slider;
	CStatusBar        m_wndStatusBar;
	CDrawWnd    m_wndView;
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
	afx_msg void OnFilePause();
	afx_msg void OnUpdateFilePause(CCmdUI *pCmdUI);
	afx_msg void OnSeekBackward();
	afx_msg void OnSeekForward();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnNMReleasedcaptureSeekbar(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTRBNThumbPosChangingSeekbar(NMHDR *pNMHDR, LRESULT *pResult);
};


