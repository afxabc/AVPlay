// VolumeDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "AVPlayer.h"
#include "VolumeDlg.h"
#include "afxdialogex.h"

using namespace eko;
// CVolumeDlg �Ի���

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


// CVolumeDlg ��Ϣ�������


BOOL CVolumeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	slider_.SetRange(0, 100);
	slider_.setWndCallback(this);

//	slider_.setSelectMin(30);
//	slider_.setSelectMax(80);
	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	return TRUE;  // return TRUE unless you set the focus to a control
				  // �쳣: OCX ����ҳӦ���� FALSE
}

void CVolumeDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);

	// TODO: �ڴ˴������Ϣ����������
//	LOGW("VolumeDlg OnActivate %d", nState);
	if (1 != nState && pWndOther != &slider_)
		ShowWindow(SW_HIDE);
}

void CVolumeDlg::OnKillFocus(CWnd* pNewWnd)
{
	CDialogEx::OnKillFocus(pNewWnd);

	// TODO: �ڴ˴������Ϣ����������
//	LOGW("VolumeDlg OnKillFocus!!!");
//	this->OnClose();
}

void CVolumeDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: �ڴ˴������Ϣ����������
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
