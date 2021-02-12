
// Voice Activity Detection ProjectDlg.h : header file
//

#pragma once
#include "SpeechDetector.h"

enum {
	WM_RECIEVE_MSG = 1756
};

// CVoiceActivityDetectionProjectDlg dialog
class CVoiceActivityDetectionProjectDlg : public CDialogEx
{
// Construction
public:
	CVoiceActivityDetectionProjectDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VOICEACTIVITYDETECTIONPROJECT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	std::unique_ptr<VAD::SpeechDetector> m_VoiceDetector;
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnRecievedMessage(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedFileAnalyze();
	BOOL m_bAbortDemux{ FALSE };
	CMFCEditBrowseCtrl m_FileBrowseCtrl;
	CString m_strFilePath;
	CEdit m_EditScanResult;
	CString m_strScanResult;
	afx_msg void OnBnClickedClearResult();
	afx_msg void OnBnClickedStopRecognize();
};
