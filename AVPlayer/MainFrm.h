
// MainFrm.h : CMainFrame 类的接口
//

#pragma once

#include "DrawWnd.h"
#include "Player.h"
#include "VolumeDlg.h"
#include "ZSliderCtrl.h"
#include "base\queue.h"

class CMainFrame : public CFrameWnd, IDrawWndCallback
{
	
public:
	CMainFrame(LPCTSTR fipath = NULL);
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// 特性
public:
	virtual void OnResetSize(int width, int height); 
	virtual void OnResetSizeFullScreen(int width, int height);
	virtual void ReportParams(int scale, ROTATIONTYPE rotate, POINT pos, SIZE szFrm, SIZE szWnd);

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
	ZSliderCtrl		  m_slider;
	CStatusBar        m_wndStatusBar;
	CDrawWnd    m_wndView;
	CSize	frmSize_;
	Player player_; 
	CVolumeDlg volDlg_;
	CString fipath_;
	Queue<FrameData> frms_;
	std::atomic_int32_t cmdPending_;

	CToolTipCtrl tooltip_;

// 生成的消息映射函数
protected:
	bool checkFilePath();
	void resizeSlider();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnFileOpen();
	afx_msg void OnFilePlay();
	afx_msg void OnPlayStartFile();
	afx_msg void OnDrawFrame();
	afx_msg void OnUpdateFilePlay(CCmdUI *pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnFilePause();
	afx_msg void OnUpdateFilePause(CCmdUI *pCmdUI);
	afx_msg void OnSeekBackward();
	afx_msg void OnSeekForward();
	afx_msg void OnUpdateSeekBackward(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSeekForward(CCmdUI *pCmdUI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVolume();
	afx_msg void OnVolumeDown();
	afx_msg void OnVolumeUp();
	afx_msg void OnFrameSave();
	afx_msg void OnUpdateFrameSave(CCmdUI *pCmdUI);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg LRESULT OnSliderChange(WPARAM w, LPARAM l);
	afx_msg LRESULT OnSliderHover(WPARAM w, LPARAM l);
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnUpdateVolume(CCmdUI *pCmdUI);
};


