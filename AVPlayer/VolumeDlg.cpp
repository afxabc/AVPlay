// VolumeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "AVPlayer.h"
#include "VolumeDlg.h"
#include "afxdialogex.h"

using namespace eko;
// CVolumeDlg 对话框

IMPLEMENT_DYNAMIC(CVolumeDlg, CDialogEx)

CVolumeDlg::CVolumeDlg()
	: CDialogEx(IDD_VOLUME_DLG, NULL)
	, player_(NULL)
{

}

CVolumeDlg::~CVolumeDlg()
{
}

BOOL CVolumeDlg::Create(Player* player, CWnd * parent)
{
	player_ = player;
	return CDialogEx::Create(IDD_VOLUME_DLG, parent);
}

void CVolumeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VOL_VALUE, statVol_);
	DDX_Control(pDX, IDC_SLIDER_VOLUME, slider_);
}


BEGIN_MESSAGE_MAP(CVolumeDlg, CDialogEx)
	ON_WM_ACTIVATE()
	ON_WM_KILLFOCUS()
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(WM_SLIDER_CHANGED, &CVolumeDlg::OnSliderChange)
END_MESSAGE_MAP()


// CVolumeDlg 消息处理程序


BOOL CVolumeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	slider_.SetRange(0, 100);
	slider_.setWndCallback(this);

//	slider_.setSelectMin(30);
//	slider_.setSelectMax(80);
	// TODO:  在此添加额外的初始化
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CVolumeDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 在此处添加消息处理程序代码
//	LOGW("VolumeDlg OnActivate %d", nState);
	if (1 != nState && pWndOther != &slider_)
		ShowWindow(SW_HIDE);
}

void CVolumeDlg::OnKillFocus(CWnd* pNewWnd)
{
	CDialogEx::OnKillFocus(pNewWnd);

	// TODO: 在此处添加消息处理程序代码
//	LOGW("VolumeDlg OnKillFocus!!!");
//	this->OnClose();
}

void CVolumeDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
	if (!bShow || player_ == NULL)
		return;

	slider_.SetPos(player_->getVolume());

	CString txt;
	txt.Format("%d", player_->getVolume());
	statVol_.SetWindowText(txt);
}

LRESULT CVolumeDlg::OnSliderChange(WPARAM w, LPARAM l)
{
	CWnd* pwnd = (CWnd*)l;

	if (pwnd == &slider_)
	{
		int pos = (int)w;
		player_->setVolume(pos);
		CString txt;
		txt.Format("%d", player_->getVolume());
		statVol_.SetWindowText(txt);
	}

	return 0;
}
