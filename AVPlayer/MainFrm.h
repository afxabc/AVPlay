
// MainFrm.h : CMainFrame ��Ľӿ�
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

// ����
public:
	virtual void OnResetSize(int width, int height);
	virtual void ReportParams(float x, float y, float scale, float rotate, int width, int height, const RECT& r);

// ����
public:

// ��д
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// ʵ��
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void onFrameData(FrameData frm); 
	void print(Log::LEVEL level, const char * sformat);

protected:  // �ؼ���Ƕ���Ա
	CToolBar          m_wndToolBar;
	CStatusBar        m_wndStatusBar;
//	CChildView    m_wndView;
//	CDrawWnd    m_wndView;
	CDrawWndD3d    m_wndView;
	Player player_;
	CString fipath_;
	Queue<FrameData> frms_;

// ���ɵ���Ϣӳ�亯��
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


