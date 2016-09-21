#pragma once


#include "Player.h"
#include "afxwin.h"
#include "afxcmn.h"
#include "ZSliderCtrl.h"
// CVolumeDlg �Ի���

class CVolumeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CVolumeDlg)

public:
	CVolumeDlg();   // ��׼���캯��
	virtual ~CVolumeDlg();
	BOOL Create(Player* player, CWnd* parent);

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VOLUME_DLG };
#endif

protected:
	Player* player_;
	ZSliderCtrl slider_;
	CStatic statVol_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg LRESULT OnSliderChange(WPARAM w, LPARAM l);
};
