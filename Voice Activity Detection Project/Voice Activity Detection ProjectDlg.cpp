
// Voice Activity Detection ProjectDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Voice Activity Detection Project.h"
#include "Voice Activity Detection ProjectDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
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


// CVoiceActivityDetectionProjectDlg dialog



CVoiceActivityDetectionProjectDlg::CVoiceActivityDetectionProjectDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VOICEACTIVITYDETECTIONPROJECT_DIALOG, pParent)
	, m_strFilePath(_T(""))
	, m_strScanResult(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVoiceActivityDetectionProjectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILE_BROWSE, m_FileBrowseCtrl);
	DDX_Text(pDX, IDC_FILE_BROWSE, m_strFilePath);
	DDX_Control(pDX, IDC_SCAN_RESULT, m_EditScanResult);
	DDX_Text(pDX, IDC_SCAN_RESULT, m_strScanResult);
}

BEGIN_MESSAGE_MAP(CVoiceActivityDetectionProjectDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_FILE_ANALYZE, &CVoiceActivityDetectionProjectDlg::OnBnClickedFileAnalyze)
	ON_MESSAGE(WM_RECIEVE_MSG, &OnRecievedMessage)
	ON_BN_CLICKED(IDC_CLEAR_RESULT, &CVoiceActivityDetectionProjectDlg::OnBnClickedClearResult)
	ON_BN_CLICKED(IDC_STOP_RECOGNIZE, &CVoiceActivityDetectionProjectDlg::OnBnClickedStopRecognize)
END_MESSAGE_MAP()


// CVoiceActivityDetectionProjectDlg message handlers

BOOL CVoiceActivityDetectionProjectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVoiceActivityDetectionProjectDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVoiceActivityDetectionProjectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVoiceActivityDetectionProjectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static inline int GetThreadMessageQueue(void* p)
{
	auto vad_p = reinterpret_cast<VAD::SpeechDetector *>(p);
	if (!vad_p)
		return -1;

	CWnd* pMainWnd = AfxGetApp()->GetMainWnd();
	if (!pMainWnd)
		return -1;

	std::unique_ptr<VAD::MessageData> msg;

	while (vad_p->getMessageQueue().pop(msg)) {

		switch (msg->msgID) {
		case VAD::MessageID::MSG_PUSH:
			pMainWnd->SendMessage(WM_RECIEVE_MSG, 0, reinterpret_cast<LPARAM>(&msg->msgText));
			break;
		case VAD::MessageID::MSG_STOP:
			return 0;
		default:
			break;
		}
	}

	return 0;

}

LRESULT CVoiceActivityDetectionProjectDlg::OnRecievedMessage(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	//UNREFERENCED_PARAMETER(lParam);
	auto text = reinterpret_cast<std::shared_ptr<char>*>(lParam);
	if (text && text->get())
	{
		UpdateData();
		m_strScanResult.Append(CA2W(text->get(), CP_UTF8));
		m_strScanResult.Append(_T("\r\n"));
		UpdateData(FALSE);
	}
	return 1;
}

static inline int DemuxCallback(void* p)
{
	auto pdlg = static_cast<CVoiceActivityDetectionProjectDlg*>(p);
	return pdlg->m_bAbortDemux ? 1 : 0;
}

void CVoiceActivityDetectionProjectDlg::OnBnClickedFileAnalyze()
{
	UpdateData();

	m_VoiceDetector.reset(new VAD::SpeechDetector);
	if (m_VoiceDetector->createVAD(CW2A(m_strFilePath, CP_UTF8).m_psz, &DemuxCallback, this))
	{
		if (m_VoiceDetector->createMessageQueue(GetThreadMessageQueue))
		{
			if (m_VoiceDetector->startRecognizeSpeech())
			{

			}
		}
	}

	UpdateData(FALSE);
}


void CVoiceActivityDetectionProjectDlg::OnBnClickedClearResult()
{
	UpdateData();
	m_strScanResult = _T("");
	UpdateData(FALSE);
}


void CVoiceActivityDetectionProjectDlg::OnBnClickedStopRecognize()
{
	m_bAbortDemux = TRUE;
	m_VoiceDetector.reset(nullptr);
}
