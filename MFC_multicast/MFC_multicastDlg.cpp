
// MFC_multicastDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "MFC_multicast.h"
#include "MFC_multicastDlg.h"
#include "afxdialogex.h"

// 定义网络事件通知消息
#define WM_SOCKET_MC WM_USER + 101
#define WM_SOCKET_MSG WM_USER + 102
#define BUFFER_SIZE 512
#define MCASTADDR "236.0.0.1"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCmulticastDlg 对话框



CMFCmulticastDlg::CMFCmulticastDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFC_MULTICAST_DIALOG, pParent), mInputStr(_T(""))
	, mLogStr(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCmulticastDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_INPUT, mInputStr);
	DDV_MaxChars(pDX, mInputStr, 500);
	DDX_Text(pDX, IDC_EDIT_LOG, mLogStr);
	DDV_MaxChars(pDX, mLogStr, 10000);
}

BEGIN_MESSAGE_MAP(CMFCmulticastDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_MSG, &CMFCmulticastDlg::OnBnClickedButtonMsg)
	ON_BN_CLICKED(IDC_BUTTON_FILE, &CMFCmulticastDlg::OnBnClickedButtonFile)
	ON_BN_CLICKED(IDC_BUTTON_FLUSH, &CMFCmulticastDlg::OnBnClickedButtonFlush)
END_MESSAGE_MAP()


// CMFCmulticastDlg 消息处理程序

BOOL CMFCmulticastDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	mListHost.SubclassDlgItem(IDC_LIST_HOST, this);
	LONG lStyle;
	CString head = _T("ip");
	lStyle = GetWindowLong(mListHost.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT; //设置style
	SetWindowLong(mListHost.m_hWnd, GWL_STYLE, lStyle);//设置style
	mListHost.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES/*|LVS_EX_CHECKBOXES*/); //设置扩展风格
	mListHost.InsertColumn(0,head,LVCFMT_LEFT,175);//插入列
	if (gethostname(hostname, 128) != 0) {
		CString msg = _T("获取用户名失败");
		::AfxMessageBox(msg);
	}
	pHost = gethostbyname(hostname);
	UDPMulticast();	//初始化组播网络发现
	SendMC('J');	//发送组播消息,J取"JOIN"
	if (!CreateServer()) {
		::AfxMessageBox(_T("初始化失败"));
	}
	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}
void CMFCmulticastDlg::AddList(sockaddr_in sa) {
	for (int i = 0; i < s_List.GetCount(); i++) {	//已经有该IP则return
		if (s_List.GetAt(i).sin_addr.S_un.S_addr == sa.sin_addr.S_un.S_addr)
			return;
	}
	s_List.Add(sa);
	int index = mListHost.GetItemCount();
	CString addr(inet_ntoa(sa.sin_addr));
	int nRow = mListHost.InsertItem(index, addr);//插入行
}
BOOL CMFCmulticastDlg::UDPMulticast() {
	if (socketMC == INVALID_SOCKET) {
		printf("socket failed with:%d\n", WSAGetLastError());
		return 0;
	}
	int ret;
	if ((sock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF |
		WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {

		printf("socket failed with:%d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(MCASTPORT);
	local.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(sock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR) {
		printf("bind failed with :%d \n", WSAGetLastError());
		closesocket(sock);
		WSACleanup();
		return -1;
	}
	//加入组播
	remote.sin_family = AF_INET;
	remote.sin_port = htons(MCASTPORT);
	remote.sin_addr.s_addr = inet_addr(MCASTADDR);
	if ((socketMC = WSAJoinLeaf(sock, (SOCKADDR *)&remote, sizeof(remote), NULL, NULL, NULL, NULL, JL_BOTH)) == INVALID_SOCKET) {
		printf("WSAJoinLeaf() failed : %d\n", WSAGetLastError());
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	return 1;
}
//发送组播
BOOL CMFCmulticastDlg::SendMC(char c) {
	char buff[2] = { c, '\0' };		//组播网络发现标识
	int ret = sendto(sock, buff, 2, 0, (struct sockaddr*)&remote, sizeof(remote));
	return ret== 2 ? TRUE : FALSE;
}
BOOL CMFCmulticastDlg::CreateServer() {
	if ((socketMsg = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF |
		WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {

		printf("socket failed with:%d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(MSGPORT);
	localaddr.sin_addr.S_un.S_addr = INADDR_ANY;
	//绑定地址
	if (bind(socketMsg, (struct sockaddr *)&localaddr, sizeof(localaddr)) == SOCKET_ERROR) return FALSE;
	//设置异步模式
	//if (::WSAAsyncSelect(socketMsg, m_hWnd, WM_SOCKET_MSG, FD_READ | FD_CLOSE) == SOCKET_ERROR) return FALSE;
	return TRUE;
}
long CMFCmulticastDlg::OnSocketMC(WPARAM wParam, LPARAM lParam) {
	SOCKET s = wParam;
	if (WSAGETSELECTERROR(lParam)) {
		::closesocket(s);
		return 0;//TODO
	}
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_READ:
	{
		//TODO
		sockaddr_in from;
		int fromlen = sizeof(from);
		char recv[2];
		memset(recv, 0, sizeof(char) * 2);
		if (::recvfrom(s, recv, sizeof(recv), 0, (sockaddr*)&from, &fromlen) == -1) {
			::closesocket(s);
			break;
		}
		if (recv[0] == 'R' || recv[0] == 'J') {
			AddList(from);	//将IP加入列表中
		}
		if (recv[0] == 'J') {
			//JOIN,发送回应RESPONSE
			char buffT[2] = { 'R', '\0' };
			::sendto(s, buffT, strlen(buffT), 0, (struct sockaddr*)&from, sizeof(from));
		}
		else if (recv[0] == 'Q') {	//Quit退出，删除对应IP
			for (int i = 0; i < s_List.GetCount(); i++) {
				if (s_List.GetAt(i).sin_addr.S_un.S_addr == from.sin_addr.S_un.S_addr) {
					s_List.RemoveAt(i);
					mListHost.DeleteItem(i);
					break;
				}
			}
		}
	}
	break;
	case FD_CLOSE:
		::closesocket(s);
		break;
	}

	return 0;
}
long CMFCmulticastDlg::OnSocketMSG(WPARAM wParam, LPARAM lParam) {
	SOCKET s = wParam;
	if (WSAGETSELECTERROR(lParam)) {
		::closesocket(s);
		return 0;//TODO
	}
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_READ:
	{
		//TODO
		UpdateData();
		sockaddr_in from;
		int fromlen = sizeof(from);
		char recv[MAX_INPUT];
		memset(recv, 0, sizeof(char) * MAX_INPUT);
		if (::recvfrom(s, recv, sizeof(recv), 0, (sockaddr*)&from, &fromlen) == -1) {
			::closesocket(s);
			break;
		}
		CString s(recv);
		int n = s.Find(':');
		CString s1 = s.Left(n);
		CString s2 = s.Right(s.GetLength() - n - 1);
		if (s1 == "FILESEND") {			//接收文件消息
			::AfxMessageBox(s2);
			char fileName[100] = "";
			OPENFILENAME file = { 0 };
			file.lStructSize = sizeof(file);
			file.lpstrFile = (LPWSTR)fileName;
			file.nMaxFile = 100;
			file.lpstrFilter = (LPWSTR)"All Files(*.*)\0*.*\0\0";
			file.nFilterIndex = 1;
			if (!::GetSaveFileName(&file))
				return 0;
			CreateFClient(fileName, from);

		}
		if (mLogStr.GetLength() + 300 > 10000)
			mLogStr.Delete(0, 300);
		mLogStr += "[";					//接收消息并且更新Log
		mLogStr += inet_ntoa(from.sin_addr);
		mLogStr += "]说：\r\n	";
		mLogStr += recv; mLogStr += "\r\n\r\n";
		UpdateData(FALSE);
	}
	break;
	case FD_CLOSE:
		::closesocket(s);
		break;
	}
	return 0;
}

void CMFCmulticastDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCmulticastDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFCmulticastDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMFCmulticastDlg::OnBnClickedButtonMsg()
{
	if (mListHost.GetFirstSelectedItemPosition() == NULL) return;
	//获取输入框文本
	UpdateData();
	CString s ("你对");
	char* buff = (LPSTR)(LPCTSTR)mInputStr;
	int len = mInputStr.GetLength();
	// 查找选中的IP
	for (int i = 0; i < mListHost.GetItemCount(); i++)
	{
		int ii;
		if ((ii = mListHost.GetItemState(i, LVIS_SELECTED))
			== LVIS_SELECTED /*|| mListHost.GetCheck(i)*/)
		{
			sockaddr_in remoteT /*= s_List.GetAt(i)*/;
			remoteT.sin_addr.S_un.S_addr = (s_List.GetAt(i)).sin_addr.S_un.S_addr;
			remoteT.sin_family = AF_INET;
			remoteT.sin_port = htons(MSGPORT);
			memset(remoteT.sin_zero, 0, 8);
			int ret = sendto(socketMsg, buff, len, 0, (sockaddr*)&remoteT, sizeof(remoteT));	//发消息
			s += "["; s += inet_ntoa(s_List.GetAt(i).sin_addr); s += "]";
		}
	}
	s += "说：\r\n	";
	if (mLogStr.GetLength() + s.GetLength() > 10000)
		mLogStr.Delete(0, s.GetLength());
	mLogStr += (s + mInputStr + _T("\r\n\r\n"));
	mInputStr = "";
	UpdateData(FALSE);
}



void CMFCmulticastDlg::OnBnClickedButtonFile()
{
	char fileName[100] = "";
	OPENFILENAME file = { 0 };
	file.lStructSize = sizeof(file);
	file.lpstrFile = (LPWSTR)fileName;
	file.nMaxFile = 100;
	file.lpstrFilter = (LPWSTR)"All Files(*.*)\0*.*\0\0";
	file.nFilterIndex = 1;
	if (!::GetOpenFileName(&file)) {
		return;
	}

	CString buff("FILESEND"); buff += ":";
	buff += inet_ntoa(*(struct in_addr*)pHost->h_addr_list[0]);
	buff += "向你发送文件 "; buff += fileName;
	int len = buff.GetLength();
	for (int i = 0; i < mListHost.GetItemCount(); i++)
	{
		int ii;
		if ((ii = mListHost.GetItemState(i, LVIS_SELECTED))
			== LVIS_SELECTED /*|| mListHost.GetCheck(i)*/)
		{
			sockaddr_in remoteT /*= s_List.GetAt(i)*/;
			remoteT.sin_addr.S_un.S_addr = (s_List.GetAt(i)).sin_addr.S_un.S_addr;
			//::AfxMessageBox(inet_ntoa(s_List.GetAt(i).sin_addr));
			remoteT.sin_family = AF_INET;
			remoteT.sin_port = htons(MSGPORT);
			memset(remoteT.sin_zero, 0, 8);
			char *buffer = (char*)(LPCTSTR)buff;
			int ret = sendto(socketMsg, buffer, len, 0, (sockaddr*)&remoteT, sizeof(remoteT));//建立文件连接，等待对方同意
			CreateFServer(fileName);
			break;
		}
	}
}
BOOL CMFCmulticastDlg::CreateFServer(char * file_name) {
	//char file_name[FILE_NAME_MAX_SIZE+1]= fName;
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	server_addr.sin_port = htons(FILEPORT);

	// 创建socket 
	SOCKET m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == m_Socket)
	{
		::AfxMessageBox(_T("Create Socket Error!")); //closesocket(c_Socket); 
		return 0;
	}

	//绑定socket和服务端(本地)地址 
	if (SOCKET_ERROR == bind(m_Socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)))
	{
		::AfxMessageBox(_T("Server Bind Failed: ") + WSAGetLastError());
		return 0;
	}

	//监听 
	if (SOCKET_ERROR == listen(m_Socket, 10))
	{
		::AfxMessageBox(_T("Server Listen Failed: ") + WSAGetLastError()); closesocket(m_Socket);
		return 0;
	}

	::AfxMessageBox(_T("点确定开始传送..\n"));

	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);

	m_Socket = accept(m_Socket, (sockaddr *)&client_addr, &client_addr_len);
	if (SOCKET_ERROR == m_Socket)
	{
		::AfxMessageBox(_T("Server Accept Failed: ") + WSAGetLastError()); closesocket(m_Socket);
		return 0;
	}

	char buffer[BUFFER_SIZE];

	memset(buffer, 0, BUFFER_SIZE);


	FILE * fp = fopen(file_name, "rb"); //windows下是"rb",表示打开一个只读的二进制文件 
	if (NULL == fp)
	{
		::AfxMessageBox(_T("File:  Not Found\n"));
	}
	else
	{
		memset(buffer, 0, BUFFER_SIZE);
		int length = 0;

		while ((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
		{
			if (send(m_Socket, buffer, length, 0) < 0)
			{
				::AfxMessageBox(_T("Send File:  Failed\n")); closesocket(m_Socket);
				return 0;
			}
			memset(buffer, 0, BUFFER_SIZE);
		}

		fclose(fp);
		closesocket(m_Socket);
	}

	::AfxMessageBox(_T("File:  Transfer Successful!\n"));

	return 1;
}
BOOL CMFCmulticastDlg::CreateFClient(char * file_name, sockaddr_in addr) {
	//创建socket 
	SOCKET c_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == c_Socket)
	{
		::AfxMessageBox(_T("Create Socket Error!")); closesocket(c_Socket);
		return 0;
	}

	//指定服务端的地址 
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = addr.sin_addr.S_un.S_addr;
	server_addr.sin_port = htons(FILEPORT);

	if (SOCKET_ERROR == connect(c_Socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)))
	{
		::AfxMessageBox(_T("Can Not Connect To Server IP!\n"));
		closesocket(c_Socket);
		return 0;
	}

	//输入文件名 
	//char file_name[FILE_NAME_MAX_SIZE+1]="D:/1.jpg"; 

	char buffer[BUFFER_SIZE];
	FILE * fp = fopen(file_name, "wb"); //windows下是"wb",表示打开一个只写的二进制文件 
	if (NULL == fp)
	{
		::AfxMessageBox(_T("File:  Can Not Open To Write\n"));
	}
	else
	{
		memset(buffer, 0, BUFFER_SIZE);
		int length = 0;
		while ((length = recv(c_Socket, buffer, BUFFER_SIZE, 0)) > 0)
		{
			if (fwrite(buffer, sizeof(char), length, fp) < length)
			{
				::AfxMessageBox(_T("File:  Write Failed\n"));
				fclose(fp);
				closesocket(c_Socket);
				return 0;
			}
			memset(buffer, 0, BUFFER_SIZE);
		}
		//TODO
	}

	::AfxMessageBox(_T("Receive File:  From client Successful!\n"));
	fclose(fp);
	closesocket(c_Socket);
	return 1;
}

void CMFCmulticastDlg::OnBnClickedButtonFlush()
{
	s_List.RemoveAll();
	mListHost.DeleteAllItems();
	if (socketMC == INVALID_SOCKET)
	{
		::AfxMessageBox(_T("组播套接字失败"));
	}
	if (!SendMC('J')) ::AfxMessageBox(_T("组播发送失败"));
}
void CMFCmulticastDlg::OnCancel()
{
	//TODO 
	SendMC('Q');
	if (socketMC != INVALID_SOCKET)
		::closesocket(socketMC);
	if (socketMsg != INVALID_SOCKET)
		::closesocket(socketMsg);
	::WSACleanup();
	CDialog::OnCancel();
}