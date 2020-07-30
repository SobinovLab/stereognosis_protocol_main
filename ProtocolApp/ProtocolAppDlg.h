
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
#include "CameraClient.h"

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
	//virtual void OnCancel() override;

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

	// camera
	CEdit m_serverStatusCtrl1;
	CEdit m_serverLogCtrl1;
	CEdit m_serverStatusCtrl2;
	CEdit m_serverLogCtrl2;

	///////// running and controlling the protocol
	std::thread * protocolThread;
	Protocol m_protocol;
	atomic<bool> m_stopProtocol;
	atomic<bool> m_startTrial;
	atomic<bool> m_stopTrial;
	void stopProtocolThread();

	void retreatStopRecording();

	//////// cameras
	CameraClient m_cameraClient1;
	atomic<bool> m_startCcRecording1;
	CameraClient m_cameraClient2;
	atomic<bool> m_startCcRecording2;
	void sendConfig();
	void syncTime();
	void sendPrepareRecording();
	void sendStartRecording();

	//////// Local devices
	NIUsb6001card m_NIUsb6001card;
	
	//////// Debug/testing controls

	/////// Enable/disable fields
	void enableProtocolCtrls(bool enable);
	void enableTrialCtrls(bool enable);
	void enableRewardCtrls(bool enable);
	void enableCameraServer1Ctrls(bool enable);
	void enableCameraServer2Ctrls(bool enable);

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

	// server
	afx_msg void OnConnect1BtnClicked();
	afx_msg void OnDisconnect1BtnClicked();
	afx_msg void OnConnect2BtnClicked();
	afx_msg void OnDisconnect2BtnClicked();
	afx_msg void OnSendConfigBtnClicked();
	afx_msg void OnSyncTimeBtnClicked();
	afx_msg void OnCaptureSingleFrameBtnClicked();
	
	DECLARE_MESSAGE_MAP()
public:
};