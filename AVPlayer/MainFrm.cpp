
// MainFrm.cpp : CMainFrame ���ʵ��
//

#include "stdafx.h"
#include "AVPlayer.h"

#include "MainFrm.h"

#include "base\astring.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_FILE_OPEN, &CMainFrame::OnFileOpen)
	ON_COMMAND(ID_FILE_PLAY, &CMainFrame::OnFilePlay)
	ON_COMMAND(IDOK, &CMainFrame::OnDrawFrame)
	ON_UPDATE_COMMAND_UI(ID_FILE_PLAY, &CMainFrame::OnUpdateFilePlay)
	ON_WM_CLOSE()
	ON_COMMAND(ID_FILE_PAUSE, &CMainFrame::OnFilePause)
	ON_UPDATE_COMMAND_UI(ID_FILE_PAUSE, &CMainFrame::OnUpdateFilePause)
	ON_COMMAND(IDC_SEEK_BACKWARD, &CMainFrame::OnSeekBackward)
	ON_COMMAND(IDC_SEEK_FORWARD, &CMainFrame::OnSeekForward)
	ON_WM_HSCROLL()
	ON_NOTIFY(NM_RELEASEDCAPTURE, ID_SEEK_BAR, &CMainFrame::OnNMReleasedcaptureSeekbar)
	ON_NOTIFY(TRBN_THUMBPOSCHANGING, ID_SEEK_BAR, &CMainFrame::OnTRBNThumbPosChangingSeekbar)
	ON_UPDATE_COMMAND_UI(IDC_SEEK_BACKWARD, &CMainFrame::OnUpdateSeekBackward)
	ON_UPDATE_COMMAND_UI(IDC_SEEK_FORWARD, &CMainFrame::OnUpdateSeekForward)
	ON_WM_SIZE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // ״̬��ָʾ��
	ID_SEPARATOR,           // 
	ID_SEPARATOR,           // 
};

// CMainFrame ����/����

CMainFrame::CMainFrame()
{
	// TODO: �ڴ���ӳ�Ա��ʼ������
}

CMainFrame::~CMainFrame()
{
	frms_.clear();
}

void CMainFrame::OnResetSize(int width, int height)
{
	CRect rChild;
	m_wndView.GetClientRect(&rChild);
	int dWidth = width - rChild.Width();
	int dHeight = height - rChild.Height();

	RECT rMain;
	this->GetWindowRect(&rMain);
	rMain.right += dWidth;
	rMain.bottom += dHeight;

	int WIDTH = GetSystemMetrics(SM_CXSCREEN);
	int HEIGHT = GetSystemMetrics(SM_CYSCREEN);

	if (WIDTH > (rMain.right - rMain.left) && HEIGHT > (rMain.bottom - rMain.top))
	{
		this->MoveWindow(&rMain);
		this->CenterWindow();
	}
	else this->ShowWindow(SW_MAXIMIZE);
	
}

void CMainFrame::ReportParams(float scale, float rotate, POINT pos, SIZE szFrm, SIZE szWnd)
{
	CString str;
	str.Format("scale=%.2f; rotate=%.2f; pos(%d, %d); szFrm(%d, %d); szWnd(%d, %d)", scale, rotate, pos.x, pos.y, szFrm.cx, szFrm.cy, szWnd.cx, szWnd.cy);
	m_wndStatusBar.SetPaneText(2, str);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// ����һ����ͼ��ռ�ÿ�ܵĹ�����
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("δ�ܴ�����ͼ����\n");
		return -1;
	}
	m_wndView.setCallback(this);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_BOTTOM | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("δ�ܴ���������\n");
		return -1;      // δ�ܴ���
	}

	m_slider.Create(WS_VISIBLE, CRect(0, 0, 10, 10), &m_wndToolBar, ID_SEEK_BAR);
//	resizeSlider();
	m_slider.SetRange(0, 100);

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("δ�ܴ���״̬��\n");
		return -1;      // δ�ܴ���
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));
	m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_NORMAL, 120);
	m_wndStatusBar.SetPaneInfo(1, ID_SEPARATOR, SBPS_NORMAL, 120);
	m_wndStatusBar.SetPaneStyle(2, SBPS_STRETCH | SBPS_NORMAL);

	// TODO: �������Ҫ��ͣ������������ɾ��������
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);


	LOGPRINT(&CMainFrame::print, this);
	player_.setDecodeFinish(boost::bind(&CMainFrame::onFrameData, this, _1));

	m_wndView.PostMessage(WM_COMMAND, IDC_INIT_SIZE);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: �ڴ˴�ͨ���޸�
	//  CREATESTRUCT cs ���޸Ĵ��������ʽ

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

// CMainFrame ���

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame ��Ϣ�������

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// ������ǰ�Ƶ���ͼ����
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// ����ͼ��һ�γ��Ը�����
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// ����ִ��Ĭ�ϴ���
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::onFrameData(FrameData frm)
{
	static const int MAX_QUEUE_SIZE = 16;
	if (frms_.size() >= MAX_QUEUE_SIZE)
		return;

	frms_.putBack(frm);
	this->PostMessage(WM_COMMAND, IDOK);
}

void CMainFrame::print(Log::LEVEL level, const char * sformat)
{
	AString str(sformat);
	str += "\n";
	TRACE(str.string());
}

bool CMainFrame::checkFilePath()
{
	CFileFind ff;
	return ff.FindFile(fipath_);
}

void CMainFrame::resizeSlider()
{
	int index = m_wndToolBar.CommandToIndex(ID_SEEK_BAR);
	if (index >= 0)
	{
		RECT rectMain;
		this->GetClientRect(&rectMain);

		RECT rectBar;
		m_wndToolBar.GetWindowRect(&rectBar);
//		rectBar.right = rectBar.left + rectMain.right - 10;
//		m_wndToolBar.MoveWindow(&rectBar);

		RECT rect;
		m_wndToolBar.GetItemRect(index, &rect);
		rect.right = rectBar.right-5;
		rect.top += 3;
		m_wndToolBar.SetButtonInfo(index, ID_SEEK_BAR, TBBS_SEPARATOR, 200);
		m_slider.MoveWindow(&rect);
	}
}

void CMainFrame::OnFileOpen()
{
	// TODO: �ڴ���������������
	if (player_.isPlaying())
		return;

	CFileDialog fdlg(TRUE);
	if (fdlg.DoModal() != IDOK)
		return;

	fipath_ = fdlg.GetPathName();
	this->SetWindowText(fdlg.GetFileTitle());
	if (player_.startPlay(fipath_))
		m_slider.SetRange(0, player_.getTimeTotal());
	m_wndView.PostMessage(WM_COMMAND, IDC_INIT_SIZE);
}

void CMainFrame::OnFilePlay()
{
	// TODO: �ڴ���������������
	if (!player_.isPlaying())
	{
		if (!checkFilePath())
			OnFileOpen();
		else player_.startPlay(fipath_);
	}
	else
	{
		player_.stopPlay();
		frms_.clear();
	}
}

void CMainFrame::OnDrawFrame()
{
	if (frms_.size() < 1)
	{
//		LOGW("û�н�ѹ֡����");
		return;
	}

	FrameData f;
	while (frms_.getFront(f))
	{
		m_wndView.DrawFrame(f);
	}

	if (GetCapture() != &m_slider)
		m_slider.SetPos(f.tm_);

	CString str;
	str.Format("%.2f / %.2f", (float)f.tm_/1000.0f, (float)player_.getTimeTotal()/1000.0f);
	m_wndStatusBar.SetPaneText(1, str);
}

void CMainFrame::OnUpdateFilePlay(CCmdUI *pCmdUI)
{
	// TODO: �ڴ������������û����洦��������
	pCmdUI->SetCheck(player_.isPlaying());
}

void CMainFrame::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	player_.stopPlay(true);
	frms_.clear();
	CFrameWnd::OnClose();
}

void CMainFrame::OnFilePause()
{
	// TODO: �ڴ���������������
	if (player_.isPaused())
		player_.setPaused(false);
	else player_.setPaused(true);
}

void CMainFrame::OnUpdateFilePause(CCmdUI *pCmdUI)
{
	// TODO: �ڴ������������û����洦��������
	pCmdUI->Enable(player_.isPlaying());
	pCmdUI->SetCheck(player_.isPaused());
}

void CMainFrame::OnUpdateSeekBackward(CCmdUI *pCmdUI)
{
	// TODO: �ڴ������������û����洦��������
	pCmdUI->Enable(player_.isPlaying());
}

void CMainFrame::OnUpdateSeekForward(CCmdUI *pCmdUI)
{
	// TODO: �ڴ������������û����洦��������
	pCmdUI->Enable(player_.isPlaying());
}

void CMainFrame::OnSeekBackward()
{
	// TODO: �ڴ���������������
	player_.seekTime(player_ .getTime()-100);
}

void CMainFrame::OnSeekForward()
{
	// TODO: �ڴ���������������
	//player_.seekTime(player_.getTime() + 200);
	player_.tickForward();
}

void CMainFrame::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMainFrame::OnNMReleasedcaptureSeekbar(NMHDR * pNMHDR, LRESULT * pResult)
{
	*pResult = 0;
	int dts = m_slider.GetPos();
	player_.seekTime(dts);

	m_wndView.SetFocus();
}

void CMainFrame::OnTRBNThumbPosChangingSeekbar(NMHDR * pNMHDR, LRESULT * pResult)
{
	*pResult = 0;
}


void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	// TODO: �ڴ˴������Ϣ����������
	resizeSlider();
}
