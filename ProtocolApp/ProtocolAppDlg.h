
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

constexpr auto FONT_TYPE = "Courier New";

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

	///////// running and controlling the protocol
	std::thread* protocolThread;  // runs Protocol::run
	Protocol m_protocol;
	void stopProtocolThread();
	void stopTrial();

public:

	//////////////// Fields
	//////// Protocol
	void toggleProtocolCtrls(bool stopped);
	void enableRewardCtrls(bool enable);
	
	// buttons

	// edits
	void setFontGuiTrialsCounter();

	// light sensors
	CStaticColor m_frontPhotoresistorCtrl;
	CStaticColor m_rearPhotoresistorCtrl;

	//////// Trial
	// buttons
	CButton m_startTrialBtn;
	CButton m_retreatBtn;
	CButton m_retreatFlushBtn;
	CButton m_loopChk;

	// edits -- mostly used via linked text
	CEdit m_trialStatus;

	//////// Pressure Sensors
	void toggleTouchServerCtrls(bool disconnected);
	
	// edits
	CEdit m_touchServerLogCtrl;

	//////// Cameras
	void toggleCameraServer1Ctrls(bool disconnected);
	void toggleCameraServer2Ctrls(bool disconnected);
	
	// edits
	CEdit m_serverLogCtrl1;
	CEdit m_serverLogCtrl2;

protected:

	/////// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	// protocol
	afx_msg void OnStartProtocolBtnClicked();
	afx_msg void OnStopProtocolBtnClicked();
	afx_msg void OnBnClickedHomeMotorsBtn();

	// reward
	afx_msg void OnFlushWaterBtnClicked();

	// trial
	afx_msg void OnStartTrialBtnClicked();
	afx_msg void OnRetreatFlushWaterBtnClicked();
	afx_msg void OnRetreatBtnClicked();

	// CAMERA server
	afx_msg void OnConnect1BtnClicked();
	afx_msg void OnDisconnect1BtnClicked();
	afx_msg void OnConnect2BtnClicked();
	afx_msg void OnDisconnect2BtnClicked();
	afx_msg void OnSendConfigBtnClicked();
	afx_msg void OnCaptureSingleFrameBtnClicked();

	// TOUCH server
	afx_msg void OnConnectTouchSensorBtnClicked();
	afx_msg void OnDisconnectTouchSensorBtnClicked();
	
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedLoopChk();
};