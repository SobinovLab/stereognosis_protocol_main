
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
	virtual BOOL CProtocolAppDlg::PreTranslateMessage(MSG* pMsg) override;
	void OnKeyPress_S();
	void OnKeyPress_T();
	void OnKeyPress_W();
	void OnKeyPress_F();

	enum UpdateDataDirection { FromVariablesToControls = FALSE, FromControlsToVariables = TRUE };

	HICON m_hIcon;

	///////// running and controlling the protocol
	std::thread* protocolThread;  // runs Protocol::run
	Protocol m_protocol;
	void stopProtocolThread();
	void stopTrial();

	// runs homing and neutral position commands
	std::thread* motorActionThread;
	void homingMotorAction();
	void neutralPositionMotorAction();
	std::atomic<bool> motorActionInProgress = false;

public:

	//////////////// Fields
	//////// Protocol
	void toggleProtocolCtrls(bool stopped);
	void enableRewardCtrls(bool enable);

	void toggleCalibrationControls(bool startedCalibration);
	
	// buttons

	// edits
	void setFontGuiTrialsCounter();

	// light sensors
	CStaticColor m_frontPhotoresistorCtrl;
	CStaticColor m_rearPhotoresistorCtrl;
	CStaticColor m_leftArmSensorCtrl;
	CStaticColor m_rightArmSensorCtrl;

	//////// Trial
	// buttons
	CButton m_startTrialBtn;
	CButton m_retreatBtn;
	CButton m_retreatFlushBtn;
	CButton m_loopChk;
	CButton m_useFrontLightSensorChk;
	CButton m_useRearLightSensorChk;
	CButton m_ledsEarlyTargetForceLightChk;

	// edits -- mostly used via linked text
	CEdit m_trialStatus;

	//////// Pressure Sensors
	void toggleTouchServerCtrls(bool disconnected);
	
	// edits
	CEdit m_touchServerLogCtrl;

	//////// Cameras
	void toggleCameraServerCtrls(bool disconnected);
	
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
	afx_msg void OnBnClickedTTLTestBtn();

	// reward
	afx_msg void OnFlushWaterBtnClicked();

	// trial
	afx_msg void OnStartTrialBtnClicked();
	afx_msg void OnRetreatFlushWaterBtnClicked();
	afx_msg void OnRetreatBtnClicked();
	afx_msg void OnConnectAllBtnClicked();
	void OnStopCalibrationBtnClicked();
	afx_msg void OnStartCalibrationBtnClicked();
	afx_msg void OnDisconnectAllBtnClicked();

	//afx_msg void OnConnect2BtnClicked();
	//afx_msg void OnDisconnect2BtnClicked();
	afx_msg void OnSendConfigBtnClicked();
	afx_msg void OnCaptureSingleFrameBtnClicked();

	// TOUCH server
	afx_msg void OnConnectTouchSensorBtnClicked();
	afx_msg void OnDisconnectTouchSensorBtnClicked();
	
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedLoopChk();
	afx_msg void OnBnClickedUseFrontLightSensorChk();
	afx_msg void OnBnClickedUseRearLightSensorChk();
	afx_msg void OnBnClickedStopMotorsBtn();
	afx_msg void OnBnClickedNeutralPositionBtn();
	afx_msg void OnBnClickedLedsEarlyTargetForceLightChk();
    afx_msg void OnEnChangeSessionFileEdt();
    afx_msg void OnBnClickedMotorsChk();


    afx_msg void OnBnClickedLeftMain();
    afx_msg void OnBnClickedLeftSecond();
    afx_msg void OnBnClickedRightMain();
    afx_msg void OnBnClickedRightSecond();
    afx_msg void OnBnClickedSplitReward();
};