
// MFC_multicastDlg.h: 头文件
//

#pragma once


// CMFCmulticastDlg 对话框
class CMFCmulticastDlg : public CDialogEx
{
// 构造
public:
	CListCtrl mListHost;
	CEdit mEditInput;
	CEdit mEditLog;
	CArray<sockaddr_in, sockaddr_in&> s_List;
	SOCKET socketMC;
	SOCKET sock;
	SOCKET socketMsg;
	SOCKET socketFile;
	struct sockaddr_in localaddr;
	struct sockaddr_in remote;
	char hostname[128];
	struct hostent*pHost;

	CString mLogStr;
	CString mInputStr;
	CMFCmulticastDlg(CWnd* pParent = nullptr);	// 标准构造函数
	void AddList(sockaddr_in);
	BOOL UDPMulticast();
	BOOL CreateMsgServer();
	BOOL SendMC(char);	//发送组播
	BOOL CreateFServer();
	BOOL recFile(char *, SOCKET);
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFC_MULTICAST_DIALOG };
#endif

// 实现
protected:
	HICON m_hIcon;
	//virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	// 套接字通知事件
	afx_msg long OnSocketMC(WPARAM wParam, LPARAM lParam);
	afx_msg long OnSocketMSG(WPARAM wParam, LPARAM lParam);
	afx_msg long CMFCmulticastDlg::OnSocketFIle(WPARAM wParam, LPARAM lParam);
	virtual void OnCancel();
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonMsg();
	afx_msg void OnBnClickedButtonFile();
	afx_msg void OnBnClickedButtonFlush();
};
