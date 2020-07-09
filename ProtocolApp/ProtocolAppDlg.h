
// ProtocolAppDlg.h : header file
//

#pragma once

#include <atomic>         // std::atomic
#include <string>
#include <thread>
#include <future>
#include "Protocol.h"
#include "ProtocolApp.h"
#include "CStaticColor.h"

#ifndef NDEBUG
	#define new DEBUG_NEW
#endif

// CProtocolAppDlg dialog
class CProtocolAppDlg : public CDialogEx
{
// Construction
public:
	CProtocolAppDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROTOCOLAPP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	enum UpdateDataDirection { FromVariablesToControls = FALSE, FromControlsToVariables = TRUE };

	HICON m_hIcon;
	
	CEdit m_rewardDurationEdtCtrl;
	CEdit m_nTrialsEdtCtrl;
	CEdit m_accelerationCtrl;
	CEdit m_speedCtrl;
	CEdit m_positionCtrl;
	CEdit m_sensorTimeTouchCtrl;
	CEdit m_intertrialTimeCtrl;
	CEdit m_currentTrialEdtCtrl;
	CEdit m_maxWaitEdtCtrl;

	CStaticColor m_frontPhotoresistorCtrl;
	CStaticColor m_rearPhotoresistorCtrl;

	std::thread * protocolThread;
	Protocol m_protocol;
	atomic<bool> m_stopProtocol;

	NIUsb6001card m_NIUsb6001card;
	void stopProtocolThread();
	// Generated message map functions
	virtual BOOL OnInitDialog();
	void enableProtocolCtrls(bool enable);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSaveProtBtnClicked();
	afx_msg void OnLoadProtBtnClicked();
	afx_msg void OnStartProtocolBtnClicked();
	afx_msg void OnStopProtocolBtnClicked();
	afx_msg void OnFlushWaterBtnClicked();
	
	DECLARE_MESSAGE_MAP()
};