
// MFC_multicast.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号
#include "afxwin.h"
#include <afxcmn.h>
#include <winsock2.h> 
#include <commctrl.h>
#include <afxtempl.h>

// CMFCmulticastApp:
// 有关此类的实现，请参阅 MFC_multicast.cpp
//
// 告诉连接器与WS2_32库连接
#pragma comment(lib,"WS2_32.lib")
// 链接到comctl32.lib库
#pragma comment(lib,"comctl32.lib")
class CMFCmulticastApp : public CWinApp
{
public:
	CMFCmulticastApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CMFCmulticastApp theApp;



