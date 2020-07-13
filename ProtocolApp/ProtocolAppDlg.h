
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
	virtual void OnOK() override;

	enum UpdateDataDirection { FromVariablesToControls = FALSE, FromControlsToVariables = TRUE };

	HICON m_hIcon;

	//////// Field edits
	// reward
	CEdit m_rewardDurationEdtCtrl;
	// protocol parameters
	CEdit m_accelerationCtrl;
	CEdit m_speedCtrl;
	CEdit m_positionCtrl;
	CEdit m_currentTrialEdtCtrl;

	// light sensors
	CStaticColor m_frontPhotoresistorCtrl;
	CStaticColor m_rearPhotoresistorCtrl;

	///////// running and controlling the protocol
	std::thread * protocolThread;
	Protocol m_protocol;
	atomic<bool> m_stopProtocol;
	atomic<bool> m_startTrial;
	atomic<bool> m_stopTrial;
	void stopProtocolThread();

	//////// Local devices
	NIUsb6001card m_NIUsb6001card;
	
	//////// Debug/testing controls

	/////// Enable/disable fields
	void enableProtocolCtrls(bool enable);
	void enableTrialCtrls(bool enable);
	void enableRewardCtrls(bool enable);
	void enableCameraControls(bool enable);
	void enableAllParameterCtrls(bool enable);

	/////// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	// load save
	afx_msg void OnSaveProtBtnClicked();
	afx_msg void OnLoadProtBtnClicked();

	// protocol
	afx_msg void OnStartProtocolBtnClicked();
	afx_msg void OnStopProtocolBtnClicked();

	// reward
	afx_msg void OnFlushWaterBtnClicked();

	// trial
	afx_msg void OnStartTrialBtnClicked();
	afx_msg void OnRetreatFlushWaterBtnClicked();
	afx_msg void OnRetreatBtnClicked();
	
	DECLARE_MESSAGE_MAP()
public:
};