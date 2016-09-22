
// MainFrm.cpp : CMainFrame 类的实现
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
	ON_COMMAND(ID_PLAY_START_FILE, &CMainFrame::OnPlayStartFile)
	ON_COMMAND(IDOK, &CMainFrame::OnDrawFrame)
	ON_UPDATE_COMMAND_UI(ID_FILE_PLAY, &CMainFrame::OnUpdateFilePlay)
	ON_WM_CLOSE()
	ON_COMMAND(ID_FILE_PAUSE, &CMainFrame::OnFilePause)
	ON_UPDATE_COMMAND_UI(ID_FILE_PAUSE, &CMainFrame::OnUpdateFilePause)
	ON_COMMAND(IDC_SEEK_BACKWARD, &CMainFrame::OnSeekBackward)
	ON_COMMAND(IDC_SEEK_FORWARD, &CMainFrame::OnSeekForward)
	ON_UPDATE_COMMAND_UI(IDC_SEEK_BACKWARD, &CMainFrame::OnUpdateSeekBackward)
	ON_UPDATE_COMMAND_UI(IDC_SEEK_FORWARD, &CMainFrame::OnUpdateSeekForward)
	ON_WM_SIZE()
	ON_COMMAND(ID_VOLUME_BAR, &CMainFrame::OnVolume)
	ON_UPDATE_COMMAND_UI(ID_VOLUME_BAR, &CMainFrame::OnUpdateVolume)
	ON_COMMAND(ID_VOLUME_DOWN, &CMainFrame::OnVolumeDown)
	ON_COMMAND(ID_VOLUME_UP, &CMainFrame::OnVolumeUp)
	ON_COMMAND(ID_FRAME_SAVE, &CMainFrame::OnFrameSave)
	ON_UPDATE_COMMAND_UI(ID_FRAME_SAVE, &CMainFrame::OnUpdateFrameSave)
	ON_WM_ACTIVATE()
	ON_MESSAGE(WM_SLIDER_CHANGED, &CMainFrame::OnSliderChange)
	ON_MESSAGE(WM_SLIDER_HOVER, &CMainFrame::OnSliderHover)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // 状态行指示器
	ID_SEPARATOR,           // 
	ID_SEPARATOR,           // 
	ID_SEPARATOR,           // 
	ID_SEPARATOR,           // 
};

// CMainFrame 构造/析构

CMainFrame::CMainFrame(LPCTSTR fipath)
	: fipath_(fipath)
	, frmSize_(-1, -1)
{
	// TODO: 在此添加成员初始化代码
}

CMainFrame::~CMainFrame()
{
	frms_.clear();
}

void CMainFrame::OnResetSize(int width, int height)
{
	CRect rMain;
	this->GetWindowRect(&rMain);

	if (frmSize_.cx < 0 && frmSize_.cy < 0)
	{
		CRect rChild;
		m_wndView.GetWindowRect(&rChild);
		frmSize_.cx = rMain.Width() - rChild.Width();
		frmSize_.cy = rMain.Height() - rChild.Height();
	}

	rMain.right = rMain.left+frmSize_.cx+width;
	rMain.bottom = rMain.top+frmSize_.cy+height;

	RECT rDesktop;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rDesktop, 0);   // 获得工作区大小
	int WIDTH = rDesktop.right;	// GetSystemMetrics(SM_CXSCREEN);
	int HEIGHT = rDesktop.bottom;	// GetSystemMetrics(SM_CYSCREEN);

	if (rMain.Width() > WIDTH)
	{
		rMain.left = 0;
		rMain.right = WIDTH;
	}

	if (rMain.Height() > HEIGHT)
	{
		rMain.top = 0;
		rMain.bottom = HEIGHT;
	}

	this->MoveWindow(&rMain);
	this->CenterWindow();
}

void CMainFrame::OnResetSizeFullScreen(int width, int height)
{
}

void CMainFrame::ReportParams(int scale, ROTATIONTYPE rotate, POINT pos, SIZE szFrm, SIZE szWnd)
{
	CString str;
//	str.Format("scale=%.2f; rotate=%.2f; pos(%d, %d); szFrm(%d, %d); szWnd(%d, %d)", scale, rotate, pos.x, pos.y, szFrm.cx, szFrm.cy, szWnd.cx, szWnd.cy);
	str.Format("%d x %d (%d%%)", szFrm.cx, szFrm.cy, scale);
	m_wndStatusBar.SetPaneText(3, str);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 创建一个视图以占用框架的工作区
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("未能创建视图窗口\n");
		return -1;
	}
	m_wndView.setCallback(this);

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("未能创建状态栏\n");
		return -1;      // 未能创建
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));
	m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_NORMAL, 100);
	m_wndStatusBar.SetPaneInfo(1, ID_SEPARATOR, SBPS_NORMAL, 100);
	m_wndStatusBar.SetPaneInfo(2, ID_SEPARATOR, SBPS_NORMAL, 100);
	m_wndStatusBar.SetPaneInfo(3, ID_SEPARATOR, SBPS_NORMAL, 120);
	m_wndStatusBar.SetPaneStyle(4, SBPS_STRETCH | SBPS_NORMAL);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_BOTTOM | CBRS_GRIPPER | CBRS_TOOLTIPS) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("未能创建工具栏\n");
		return -1;      // 未能创建
	}

	m_slider.Create(WS_VISIBLE, CRect(0, 0, 10, 10), &m_wndToolBar, ID_SEEK_BAR);
	m_slider.SetRange(0, 0);

	// TODO: 如果不需要可停靠工具栏，则删除这三行
//	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
//	EnableDocking(CBRS_ALIGN_ANY);
//	DockControlBar(&m_wndToolBar);


	LOGPRINT(&CMainFrame::print, this);
//	player_.setDecodeFinish(boost::bind(&CMainFrame::onFrameData, this, _1));
	player_.setDecodeFinish([this](FrameData frm) { this->onFrameData(frm); });

	m_wndView.PostMessage(WM_COMMAND, IDC_INIT_SIZE);

	fipath_.Replace('"', ' ');
	fipath_.Trim();
	if (fipath_.GetLength() > 1)
		PostMessage(WM_COMMAND, ID_PLAY_START_FILE);

	tooltip_.Create(this);
	tooltip_.Activate(TRUE);
	tooltip_.AddTool(&m_slider, "0000");
	tooltip_.SetDelayTime(100);

	volDlg_.Create(&player_, this);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

// CMainFrame 诊断

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


// CMainFrame 消息处理程序

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// 将焦点前移到视图窗口
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// 让视图第一次尝试该命令
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// 否则，执行默认处理
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::onFrameData(FrameData frm)
{
	static const int MAX_QUEUE_SIZE = 16;
	if (frms_.size() >= MAX_QUEUE_SIZE)
	{
		LOGW("onFrameData : queue fulled !!!!!!");
		return;
	}
		
	frms_.putBack(frm);
	this->PostMessage(WM_COMMAND, IDOK);
}

void CMainFrame::print(Log::LEVEL level, const char * sformat)
{
	AString str(sformat);
	str += "\n";
	TRACE(str.string());
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

		int barLen = rectBar.right - rectBar.left;
		
		RECT rect;
		m_wndToolBar.GetItemRect(index, &rect);

		int bnLen = barLen - rect.left - 5;
		rect.right = rect.left + bnLen - 5;
		rect.top += 3;
		m_wndToolBar.SetButtonInfo(index, ID_SEEK_BAR, TBBS_SEPARATOR, bnLen);
		m_slider.MoveWindow(&rect);
	}
}

bool CMainFrame::checkFilePath()
{
	CFileFind ff;
	if (!ff.FindFile(fipath_))
		return FALSE;

	ff.FindNextFile();
	this->SetWindowText(ff.GetFileTitle());
	return TRUE;
}

void CMainFrame::OnFileOpen()
{
	// TODO: 在此添加命令处理程序代码
	CFileDialog fdlg(TRUE);
	if (fdlg.DoModal() != IDOK)
		return;

	fipath_ = fdlg.GetPathName();
	PostMessage(WM_COMMAND, ID_PLAY_START_FILE);
}

void CMainFrame::OnPlayStartFile()
{
	if (!checkFilePath())
		return;

	if (player_.startPlay(fipath_))
	{
		m_slider.SetRange(0, player_.getTimeTotal());
		Timestamp tm = Timestamp(player_.getTimeTotal());
		CString str;
		str.Format("%s", tm.toString().c_str());
		m_wndStatusBar.SetPaneText(2, str);
	}
	m_wndView.PostMessage(WM_COMMAND, IDC_INIT_SIZE);
}

void CMainFrame::OnFilePlay()
{
	// TODO: 在此添加命令处理程序代码
	if (!player_.isPlaying())
	{
		if (!checkFilePath())
			OnFileOpen();
		else player_.startPlay(fipath_);
	}
	else
	{
		if (player_.isPaused())
			player_.setPaused(false);
		else
		{
			player_.stopPlay();
			frms_.clear();
		}
	}
}

void CMainFrame::OnDrawFrame()
{
	if (frms_.size() < 1)
	{
//		LOGW("没有解压帧！！");
		return;
	}

	FrameData f;
	while (frms_.getFront(f))
	{
		m_wndView.DrawFrame(f);
	}

	if (GetCapture() != &m_slider)
		m_slider.SetPos(f.tm_);

	Timestamp tm = Timestamp(f.tm_);
	CString str;
	str.Format("%s", tm.toString().c_str());
	m_wndStatusBar.SetPaneText(1, str);
}

void CMainFrame::OnUpdateFilePlay(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->SetCheck(player_.isPlaying());
}

void CMainFrame::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	player_.stopPlay(true);
	frms_.clear();
	CFrameWnd::OnClose();
}

void CMainFrame::OnFilePause()
{
	// TODO: 在此添加命令处理程序代码
	if (player_.isPaused())
		player_.setPaused(false);
	else player_.setPaused(true);
}

void CMainFrame::OnUpdateFilePause(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->Enable(player_.isPlaying());
	pCmdUI->SetCheck(player_.isPaused());
}

void CMainFrame::OnUpdateSeekBackward(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->Enable(player_.isPlaying());
}

void CMainFrame::OnUpdateSeekForward(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->Enable(player_.isPlaying());
}

void CMainFrame::OnSeekBackward()
{
	// TODO: 在此添加命令处理程序代码
	player_.setPaused(true);
	player_.seekTime(player_ .getTime()-100);
}

void CMainFrame::OnSeekForward()
{
	// TODO: 在此添加命令处理程序代码
	//player_.seekTime(player_.getTime() + 200);
	player_.tickForward();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	resizeSlider();
}

void CMainFrame::OnUpdateVolume(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->SetCheck(volDlg_.IsWindowVisible());
}

void CMainFrame::OnVolume()
{
	// TODO: 在此添加命令处理程序代码

	int index = m_wndToolBar.CommandToIndex(ID_VOLUME_BAR);
	RECT rect;
	m_wndToolBar.GetItemRect(index, &rect);
	m_wndToolBar. ClientToScreen(&rect);

	RECT r;
	volDlg_.GetWindowRect(&r);
	int w = r.right - r.left;
	int h = r.bottom - r.top;

	r.left = rect.left;
	r.top = rect.top - h;
	r.right = rect.left + w;
	r.bottom = rect.top;

	volDlg_.MoveWindow(&r, FALSE);
	volDlg_.ShowWindow(SW_SHOW);
}

void CMainFrame::OnVolumeDown()
{
	// TODO: 在此添加命令处理程序代码
	int vol = player_.getVolume();
	player_.setVolume(vol - 1);
}

void CMainFrame::OnVolumeUp()
{
	// TODO: 在此添加命令处理程序代码
	int vol = player_.getVolume();
	player_.setVolume(vol + 1);
}

void CMainFrame::OnFrameSave()
{
	// TODO: 在此添加命令处理程序代码
	player_.setPaused(true);

	const FrameData& frm = m_wndView.frame();
	CString finame;
	GetWindowText(finame);
	finame += " - ";
	finame += Timestamp(frm.tm_).toString().c_str();
	finame += ".jpg";
	finame.Replace(':', '_');

	static const char szFilters[] = "图像文件(*.jpg, *.jpeg)|*.jpg|*.jpeg||";
	// Create an Open dialog; the default file name extension is ".my".
	CFileDialog fileDlg(FALSE, "jpg", finame,
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, szFilters, this);

	if (fileDlg.DoModal() != IDOK)
		return;

	frm.toFileJpg(fileDlg.GetPathName());
}

void CMainFrame::OnUpdateFrameSave(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->Enable(player_.isPlaying());
}

void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	__super::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 在此处添加消息处理程序代码
	if (!bMinimized)
		m_wndView.ReDraw();
}

LRESULT CMainFrame::OnSliderChange(WPARAM w, LPARAM l)
{
	CWnd* pwnd = (CWnd*)l;

	if (pwnd == &m_slider)
	{
		int pos = (int)w;
		player_.seekTime(pos);
	}

	m_wndView.SetFocus();
	
	return 0;
}

LRESULT CMainFrame::OnSliderHover(WPARAM w, LPARAM l)
{
	CWnd* pwnd = (CWnd*)l;

	if (pwnd == &m_slider)
	{
		eko::Timestamp t(w);
		tooltip_.UpdateTipText(t.toString().c_str(), &m_slider);
	}

	return 0;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	tooltip_.RelayEvent(pMsg);
	return __super::PreTranslateMessage(pMsg);
}

