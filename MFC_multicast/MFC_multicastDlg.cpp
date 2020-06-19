
// MFC_multicastDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "MFC_multicast.h"
#include "MFC_multicastDlg.h"
#include "afxdialogex.h"

// 定义网络事件通知消息
#define WM_SOCKET_MC WM_USER + 101
#define WM_SOCKET_MSG WM_USER + 102
#define WM_SOCKET_FILE WM_USER + 103
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
	if (!(CreateMsgServer()&&CreateFServer())) {
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
	ret = ::WSAAsyncSelect(socketMC, m_hWnd, WM_SOCKET_MC, FD_READ | FD_CLOSE);	//设置异步模式
	if (ret == SOCKET_ERROR) return FALSE;
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
	std::cout << ret << std::endl;
	return ret== 2 ? TRUE : FALSE;
}
BOOL CMFCmulticastDlg::CreateMsgServer() {
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
	if (::WSAAsyncSelect(socketMsg, m_hWnd, WM_SOCKET_MSG, FD_READ | FD_CLOSE) == SOCKET_ERROR) return FALSE;
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
		struct sockaddr_in from;
		int fromlen = sizeof(from);
		char recv[2];
		memset(recv, 0, sizeof(char) * 2);
		if (::recvfrom(s, recv, sizeof(recv), 0, (struct sockaddr*)&from, &fromlen) == -1) {
			::closesocket(s);
			break;
		}
		if (recv[0] == 'R' || recv[0] == 'J') {
			AddList(from);	//将IP加入列表中
		}
		if (recv[0] == 'J') {
			//JOIN,发送回应RESPONSE
			char buffT[2] = { 'R', '\0' };
			::sendto(s, buffT, strlen(buffT), 0, (struct sockaddr*)&from, fromlen);
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
		UpdateData();
		struct sockaddr_in from;
		int fromlen = sizeof(from);
		char recv[MAX_INPUT];
		memset(recv, 0, sizeof(char) * MAX_INPUT);
		if (::recvfrom(s, recv, sizeof(recv), 0, (struct sockaddr*)&from, &fromlen) == -1) {
			::closesocket(s);
			break;
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
long CMFCmulticastDlg::OnSocketFIle(WPARAM wParam, LPARAM lParam)
{
	SOCKET s = wParam;
	if (WSAGETSELECTERROR(lParam)) {
		::closesocket(s);
		return 0;//TODO
	}
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_READ:
	{
		UpdateData();
		struct sockaddr_in from;
		int fromlen = sizeof(from);
		char recv[BUFFER_SIZE];
		memset(recv, 0, sizeof(char) * BUFFER_SIZE);
		if (::recvfrom(s, recv, sizeof(recv), 0, (struct sockaddr*)&from, &fromlen) == -1) {
			::closesocket(s);
			break;
		}
		CString e(recv);
		CString r("sendfile");
		int n = e.Find(':');
		CString s1 = e.Left(n);
		if (s1 == r)
		{
			CString s2 = e.Right(e.GetLength() - n - 1);
			::AfxMessageBox(s2);
			char fileName[100] = "";
			OPENFILENAME file = { 0 };
			file.lStructSize = sizeof(file);
			file.lpstrFile = (LPWSTR)fileName;
			file.nMaxFile = 100;
			file.lpstrFilter = (LPCWSTR)"All Files(*.*)\0*.*\0\0";
			file.nFilterIndex = 1;
			if (!::GetSaveFileName(&file))
			{
				std::cout << "获取保存文件名失败" << std::endl;
				return 0;
			}
			recFile(fileName, s);
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
	CString s ("你");
	char* buff = (LPSTR)(LPCTSTR)mInputStr;
	int len = mInputStr.GetLength();
	struct sockaddr_in remoteT;
	remoteT.sin_addr.s_addr = inet_addr(MCASTADDR);
	remoteT.sin_family = AF_INET;
	remoteT.sin_port = htons(MSGPORT);
	int ret = sendto(socketMsg, buff, len, 0, (struct sockaddr*)&remoteT, sizeof(remoteT));
	//发消息
	if(ret<=0)
		std::cout<<"消息发送失败"<<std::endl;
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
		::AfxMessageBox(_T("选择文件失败!"));
		return;
	}
	CString buff("sendfile:");
	buff += inet_ntoa(*(struct in_addr*)pHost->h_addr_list[0]);
	buff += "向你发送文件 "; buff += fileName;
	int len = buff.GetLength();
	struct sockaddr_in remoteT;
	remoteT.sin_addr.s_addr = inet_addr(MCASTADDR);
	remoteT.sin_family = AF_INET;
	remoteT.sin_port = htons(FILEPORT);
	char *buffer = (char*)(LPCTSTR)buff;
	int ret = sendto(socketFile, buffer, len, 0, (struct sockaddr*)&remoteT, sizeof(remoteT));//传输文件名
	if (ret <= 0)
	{
		std::cout << "文件名传输失败" << std::endl;
	}
	::AfxMessageBox(_T("点确定开始传送..\n"));
	char buffer2[BUFFER_SIZE];
	memset(buffer2, 0, BUFFER_SIZE);
	FILE * fp = fopen(fileName, "rb"); //windows下是"rb",表示打开一个只读的二进制文件 
	if (NULL == fp)
	{
		::AfxMessageBox(_T("File:  Not Found\n"));
	}
	else
	{
		memset(buffer2, 0, BUFFER_SIZE);
		int length = 0;
		while ((length = fread(buffer2, sizeof(char), BUFFER_SIZE, fp)) > 0)
		{
			if (sendto(socketFile, (char *)buffer2, strlen(buffer2), 0, (struct sockaddr *) &remoteT, sizeof(remoteT)) < 0)
			{
				::AfxMessageBox(_T("Send File:  Failed\n")); closesocket(socketFile);
				return;
			}
			memset(buffer2, 0, BUFFER_SIZE);
		}
		fclose(fp);
		char sendbuf[10];
		sendbuf[0] = 'Q';
		sendbuf[1] = 'U';
		sendbuf[2] = 'I';
		sendbuf[3] = 'T';
		sendbuf[4] = '\0';
		sendto(socketFile, (char *)sendbuf, strlen(sendbuf), 0, (struct sockaddr *) &remoteT, sizeof(remoteT));
		closesocket(socketFile);
	}
	::AfxMessageBox(_T("File:  Transfer Successful!\n"));
	return;
}
BOOL CMFCmulticastDlg::CreateFServer() {
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	server_addr.sin_port = htons(FILEPORT);

	// 创建socket 
	if ((socketFile = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF |
		WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {

		printf("socket failed with:%d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	//绑定socket和服务端(本地)地址 
	if (SOCKET_ERROR == bind(socketFile, (LPSOCKADDR)&server_addr, sizeof(server_addr)))
	{
		::AfxMessageBox(_T("Server Bind Failed: ") + WSAGetLastError());
		return 0;
	}
	if (::WSAAsyncSelect(socketFile, m_hWnd, WM_SOCKET_FILE, FD_READ | FD_CLOSE) == SOCKET_ERROR) return FALSE;
	return 1;
}
BOOL CMFCmulticastDlg::recFile(char * file_name, SOCKET s) {
	//指定服务端的地址 
	SOCKET n = s;
	struct sockaddr_in from;
	int flag = 0;
	int len = sizeof(struct sockaddr_in);
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
		
		while ((length = recvfrom(n, buffer, BUFFER_SIZE,0, (struct sockaddr *)&from, &len)) > 0)
		{
			if (length < 0)
			{
				printf("recvfrom failed with: %d\n", WSAGetLastError());
				closesocket(n);
				break;
			}
			if (strcmp(buffer, "QUIT") == 0)
			{
				std::cout << "结束" << std::endl;
				flag = 1;
				break;
			}
			if (fwrite(buffer, sizeof(char), length, fp) < length)
			{
				::AfxMessageBox(_T("File:  Write Failed\n"));
				fclose(fp);
				closesocket(n);
				return 0;
			}
			memset(buffer, 0, BUFFER_SIZE);
		}
	}
	if(flag==1)
	::AfxMessageBox(_T("Receive File:  From client Successful!\n"));
	fclose(fp);
	closesocket(n);
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