
// ProtocolAppDlg.h : header file
//

#pragma once

#include <atomic>         // std::atomic
#include <string>
#include <thread>
#include <future>
#include "Protocol.h"
#include "Sounds.h"
#include "ProtocolApp.h"
#include "CStaticColor.h"
#include "CameraClient.h"
#include "TouchSensorClient.h"

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

	//////// Field edits
	// reward
	CEdit m_rewardDurationEdtCtrl;
	// protocol parameters
	CEdit m_accelerationCtrl;
	CEdit m_speedCtrl;
	CEdit m_positionCtrl;
	CEdit m_currentTrialEdtCtrl;
	void setFontGuiTrialsCounter();

	// light sensors
	CStaticColor m_frontPhotoresistorCtrl;
	CStaticColor m_rearPhotoresistorCtrl;

	// camera
	CEdit m_serverStatusCtrl1;
	CEdit m_serverLogCtrl1;
	CEdit m_serverStatusCtrl2;
	CEdit m_serverLogCtrl2;

	// touch sensors
	CEdit m_touchServerLogCtrl;

	// trial loop
	CButton m_trialLoopChk;

	///////// running and controlling the protocol
	std::thread* protocolThread;
	Protocol m_protocol;
	void stopProtocolThread();

	//////// both control
	void sendStartRecording();
	void retreatStopRecording();

	//////// cameras
	CameraClient m_cameraClient1;
	CameraClient m_cameraClient2;
	void sendConfig();
	void syncTime();
	void sendPrepareRecording();

	//////// touch sensors
	TouchSensorClient m_touchSensorClient;
	std::thread* m_touchSensorSuccessMonitorThread;
	void m_touchSensorSuccessMonitor();
	std::atomic<bool> stopTouchSensorSuccessMonitor;

	//////// Local devices
	NIUsb6001card m_NIUsb6001card;

	//////// Debug/testing controls

	/////// Enable/disable fields
	void enableProtocolCtrls(bool enable);
	void enableTrialCtrls(bool enable);
	void enableRewardCtrls(bool enable);
	void enableCameraServer1Ctrls(bool enable);
	void enableCameraServer2Ctrls(bool enable);
	void enableTouchServerCtrls(bool enable);

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

	// CAMERA server
	afx_msg void OnConnect1BtnClicked();
	afx_msg void OnDisconnect1BtnClicked();
	afx_msg void OnConnect2BtnClicked();
	afx_msg void OnDisconnect2BtnClicked();
	afx_msg void OnSendConfigBtnClicked();
	afx_msg void OnSyncTimeBtnClicked();
	afx_msg void OnCaptureSingleFrameBtnClicked();

	// TOUCH server
	afx_msg void OnConnectTouchSensorBtnClicked();
	afx_msg void OnDisconnectTouchSensorBtnClicked();
	
	DECLARE_MESSAGE_MAP()
public:
};