
// AVPlayer.h : AVPlayer Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������


// CAVPlayerApp:
// �йش����ʵ�֣������ AVPlayer.cpp
//

class CAVPlayerApp : public CWinApp
{
public:
	CAVPlayerApp();


// ��д
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// ʵ��

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CAVPlayerApp theApp;
