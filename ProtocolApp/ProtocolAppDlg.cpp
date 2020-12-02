// ProtocolAppDlg.cpp : implementation file
//

#include "ProtocolAppDlg.h"

constexpr auto ENABLED = true;
constexpr auto DISABLED = false;

// CProtocolAppDlg dialog

CProtocolAppDlg::CProtocolAppDlg(CWnd* pParent /*=nullptr*/) : CDialogEx(IDD_PROTOCOLAPP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProtocolAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_REWARD_TIME_EDT, m_rewardDurationEdtCtrl);

	DDX_Control(pDX, IDC_ACCELERATION_EDT, m_accelerationCtrl);
	DDX_Control(pDX, IDC_SPEED_EDT, m_speedCtrl);
	DDX_Control(pDX, IDC_POSITION_EDT, m_positionCtrl);
	DDX_Control(pDX, IDC_CURRENT_TRIAL_EDT_BOX, m_currentTrialEdtCtrl);

	DDX_Control(pDX, IDC_PHOTORES_FRONT_LBL, m_frontPhotoresistorCtrl);
	DDX_Control(pDX, IDC_PHOTORES_REAR_LBL, m_rearPhotoresistorCtrl);

	DDX_Control(pDX, IDC_SERVER_STATUS_EDT1, m_serverStatusCtrl1);
	DDX_Control(pDX, IDC_SERVER_LOG_EDT1, m_serverLogCtrl1);
	DDX_Control(pDX, IDC_SERVER_STATUS_EDT2, m_serverStatusCtrl2);
	DDX_Control(pDX, IDC_SERVER_LOG_EDT3, m_serverLogCtrl2);

	DDX_Control(pDX, IDC_TOUCH_SENSOR_SERVER_LOG_EDT, m_touchServerLogCtrl);

	DDX_Control(pDX, IDC_LOOP_TRIALS_CHK, m_trialLoopChk);

	DDX_Text(pDX, IDC_REWARD_TIME_EDT, m_protocol.params.rewardDuration);
	DDX_Text(pDX, IDC_ACCELERATION_EDT, m_protocol.params.acceleration);
	DDX_Text(pDX, IDC_SPEED_EDT, m_protocol.params.speed);
	DDX_Text(pDX, IDC_POSITION_EDT, m_protocol.params.position);
	DDX_Text(pDX, IDC_MAX_WAIT_EDT_BOX, m_protocol.params.maxWaitTime);
	DDX_Text(pDX, IDC_INTERTRIAL_WAIT_EDT, m_protocol.params.intertrialWaitTime);

	DDX_Text(pDX, IDC_IP_EDT1, m_protocol.params.cs_ip1);
	DDX_Text(pDX, IDC_PORT_EDT1, m_protocol.params.cs_port1);
	DDX_Text(pDX, IDC_IP_EDT2, m_protocol.params.cs_ip2);
	DDX_Text(pDX, IDC_PORT_EDT2, m_protocol.params.cs_port2);
	DDX_Text(pDX, IDC_FRAMERATE_EDT, m_protocol.params.cs_framerate);
	DDX_Text(pDX, IDC_RECORDING_PERIOD_EDT, m_protocol.params.cs_recordingPeriod);
	DDX_Text(pDX, IDC_REF_SERIAL_EDT, m_protocol.params.cs_refSerial);
	DDX_Text(pDX, IDC_GAIN_EDT, m_protocol.params.cs_gain);
	DDX_Text(pDX, IDC_EXPOSURE_EDT, m_protocol.params.cs_exposure);

	DDX_Text(pDX, IDC_TOUCH_SENSOR_IP_EDT, m_protocol.params.tss_ip);
	DDX_Text(pDX, IDC_TOUCH_SENSOR_PORT_EDT, m_protocol.params.tss_port);
}

BEGIN_MESSAGE_MAP(CProtocolAppDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOAD_CONFIG_BTN, OnLoadProtBtnClicked)
	ON_BN_CLICKED(IDC_SAVE_CONFIG_BTN, OnSaveProtBtnClicked)
	ON_BN_CLICKED(IDC_FLUSH_WATER_BTN, OnFlushWaterBtnClicked)
	ON_BN_CLICKED(IDC_STOP_PROTOCOL_BTN, OnStopProtocolBtnClicked)
	ON_BN_CLICKED(IDC_START_PROTOCOL_BTN, OnStartProtocolBtnClicked)
	ON_BN_CLICKED(IDC_START_TRIAL_BTN, OnStartTrialBtnClicked)
	ON_BN_CLICKED(IDC_RETREAT_FLUSH_WATER_BTN, OnRetreatFlushWaterBtnClicked)
	ON_BN_CLICKED(IDC_RETREAT_BTN, OnRetreatBtnClicked)
	ON_BN_CLICKED(IDC_CONNECT_BTN1, OnConnect1BtnClicked)
	ON_BN_CLICKED(IDC_DISCONNECT_BTN1, OnDisconnect1BtnClicked)
	ON_BN_CLICKED(IDC_CONNECT_BTN2, OnConnect2BtnClicked)
	ON_BN_CLICKED(IDC_DISCONNECT_BTN2, OnDisconnect2BtnClicked)
	ON_BN_CLICKED(IDC_SEND_CONFIG_BTN, OnSendConfigBtnClicked)
	ON_BN_CLICKED(IDC_CAPTURE_SINGLE_FRAME_BTN, OnCaptureSingleFrameBtnClicked)
	ON_BN_CLICKED(IDC_CONNECT_TOUCH_SENSOR_BTN, OnConnectTouchSensorBtnClicked)
	ON_BN_CLICKED(IDC_DISCONNECT_TOUCH_SENSOR_BTN, OnDisconnectTouchSensorBtnClicked)
END_MESSAGE_MAP()

// CProtocolAppDlg message handlers
BOOL CProtocolAppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

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

	// Send GUI pointers to the protocol
	setFontGuiTrialsCounter();
	m_protocol.set_photoresistor_monitors(&m_frontPhotoresistorCtrl, &m_rearPhotoresistorCtrl);
	m_serverStatusCtrl1.SetWindowText("Off");
	m_protocol.set_camera1_gui_controls(&m_serverStatusCtrl1, &m_serverLogCtrl1);
	m_serverStatusCtrl2.SetWindowText("Off");
	m_protocol.set_camera2_gui_controls(&m_serverStatusCtrl2, &m_serverLogCtrl2);

	// set the visibility and defaults for GUI
	if (m_protocol.params.tstEnLightSensors) ((CButton*)GetDlgItem(IDC_LIGHT_SENSORS_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.params.tstEnMotors) ((CButton*)GetDlgItem(IDC_MOTORS_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.params.tstEnReward) ((CButton*)GetDlgItem(IDC_REWARD_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.params.tstEnTouchSensors) ((CButton*)GetDlgItem(IDC_TOUCH_SENSORS_CHK))->SetCheck(BST_CHECKED);

	m_trialLoopChk.SetCheck(true);

	/////// Control what is enabled and initialized based on debug/testing interface
	enableProtocolCtrls(true);
	enableTrialCtrls(true);
	GetDlgItem(IDC_START_TRIAL_BTN)->EnableWindow(false);

	// motors
	// light sensors
	// touch sensors
	// All motors, light, touch go through NI card automatically

	// reward
	enableRewardCtrls(true);

	// cameras
	enableCameraServer1Ctrls(true);
	enableCameraServer2Ctrls(true);

	// touch sensor
	enableTouchServerCtrls(true);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CProtocolAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	UINT command = (nID & 0xFFF0);

	if (command == SC_CLOSE) {
		OnStopProtocolBtnClicked();
	}
	CDialogEx::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CProtocolAppDlg::OnPaint()
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

void CProtocolAppDlg::OnOK()
{
	UpdateData(FromControlsToVariables);
}

void CProtocolAppDlg::setFontGuiTrialsCounter()
{
	CFont* cEditControlFont = new CFont();
	cEditControlFont->CreateFont(30, 0, 0, 0, FW_HEAVY, true, false, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(FONT_TYPE));
	m_currentTrialEdtCtrl.SetFont(cEditControlFont);
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CProtocolAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CProtocolAppDlg::OnStartProtocolBtnClicked()
{
	UpdateData(FromControlsToVariables);
	enableProtocolCtrls(DISABLED);
	m_protocol.stopProtocol.store(false);
	m_protocol.startTrial.store(false);
	m_protocol.stopTrial.store(false);
	m_protocol.retreatedMotors.store(false);

	protocolThread = new thread(&Protocol::run, &m_protocol, &m_currentTrialEdtCtrl);

	enableTrialCtrls(true);
}

void CProtocolAppDlg::OnStopProtocolBtnClicked()
{
	// if trials are running, uncheck loop and stop trial
	bool initState = m_trialLoopChk.GetCheck();
	m_trialLoopChk.SetCheck(false);
	if (!stopTouchSensorSuccessMonitor.load()) {
		retreatStopRecording();
	}

	stopProtocolThread();

	// trial controls fully off
	enableTrialCtrls(true);
	GetDlgItem(IDC_START_TRIAL_BTN)->EnableWindow(false);
	// protocol controls and edits are on
	enableProtocolCtrls(ENABLED);

	// restore the check button
	m_trialLoopChk.SetCheck(initState);
}

void CProtocolAppDlg::OnFlushWaterBtnClicked()
{
	UpdateData(FromControlsToVariables);
	m_protocol.reward();
}

void CProtocolAppDlg::OnStartTrialBtnClicked()
{
	enableTrialCtrls(false);
	UpdateData(FromControlsToVariables);

	m_protocol.startTrial.store(true);

	sendStartRecording();
}

void CProtocolAppDlg::OnRetreatFlushWaterBtnClicked()
{
	UpdateData(FromControlsToVariables);
	m_protocol.reward();

	retreatStopRecording();

}

void CProtocolAppDlg::OnRetreatBtnClicked()
{
	if (m_protocol.params.tstEnReward) {
		Sounds::playErrorTone();
	}

	retreatStopRecording();
}

void CProtocolAppDlg::OnConnect1BtnClicked()
{
	UpdateData(FromControlsToVariables);

	// TODO (AS) return error if not connected and not change state?
	m_protocol.connect_camera_client1();

	enableCameraServer1Ctrls(false);
}

void CProtocolAppDlg::OnDisconnect1BtnClicked()
{
	m_protocol.disconnect_camera_client1();
	enableCameraServer1Ctrls(true);
}

void CProtocolAppDlg::OnConnect2BtnClicked()
{
	UpdateData(FromControlsToVariables);

	m_protocol.connect_camera_client2();

	enableCameraServer2Ctrls(false);
}

void CProtocolAppDlg::OnDisconnect2BtnClicked()
{
	m_protocol.disconnect_camera_client2();

	enableCameraServer2Ctrls(true);
}

void CProtocolAppDlg::OnSendConfigBtnClicked()
{
	UpdateData(FromControlsToVariables);
	m_protocol.send_config_to_cameras();
}

void CProtocolAppDlg::OnCaptureSingleFrameBtnClicked()
{
	UpdateData(FromControlsToVariables);

	m_protocol.capture_single_frame();
}

void CProtocolAppDlg::OnConnectTouchSensorBtnClicked()
{
	UpdateData(FromControlsToVariables);

	m_touchSensorClient.server_ip = m_protocol.params.tss_ip;
	m_touchSensorClient.port = m_protocol.params.tss_port;
	m_touchSensorClient.clientLogGuiEdt = &m_touchServerLogCtrl;

	m_touchSensorClient.connect_f();

	enableTouchServerCtrls(false);

}

void CProtocolAppDlg::OnDisconnectTouchSensorBtnClicked()
{
	m_touchSensorClient.disconnect_f();

	enableTouchServerCtrls(true);
}

void CProtocolAppDlg::stopProtocolThread()
{
	if (protocolThread) {
		m_protocol.stopProtocol.store(true);
		protocolThread->join();
		delete protocolThread; protocolThread = nullptr;
	}

}

void CProtocolAppDlg::retreatStopRecording()
{
	stopTouchSensorSuccessMonitor.store(true);

	enableTrialCtrls(true);

	// retreat
	m_protocol.stopTrial.store(true);

	// wait until the motors fully retracted
	while (!m_protocol.retreatedMotors.load()) {}

	// and only then stop recording

	if (m_touchSensorClient.isConnected()) {
		atomic<int> result;
		m_touchSensorClient.breakRecording(&result);
	}
}

void CProtocolAppDlg::m_touchSensorSuccessMonitor()
{
	UpdateData(FromControlsToVariables);
	double maxWaitTime = m_protocol.params.maxWaitTime;
	double intertrialWaitTime = m_protocol.params.intertrialWaitTime;
	auto startTime = chrono::steady_clock::now();
	std::atomic<int> result;
	long timePassed;


	while (!stopTouchSensorSuccessMonitor.load()) {
		Sleep(50);  // ms loop, so not too often
		if (stopTouchSensorSuccessMonitor.load())
			break;

		// ask for success
		m_touchSensorClient.checkSuccess(&result);

		if (result.load() > 0) {
			// give reward
			if (m_protocol.params.tstEnReward) {
				m_protocol.reward();
			}

			// do the for trial break - this also stops this thread
			retreatStopRecording();

			if (m_trialLoopChk.GetCheck()) {
				Sleep(intertrialWaitTime * 1000);
			}

			// start the next trial
			if (m_trialLoopChk.GetCheck()) {
				OnStartTrialBtnClicked();
			}
			break;
		}
		else {
			// check if out of time and then stop, punish and retreat
			timePassed = Times::getElapsedMilliSecsSince(startTime);
			if (timePassed >= maxWaitTime * 1e3) {
				Sounds::playErrorTone();

				// do the for trial break - this also stops this thread
				retreatStopRecording();

				if (m_trialLoopChk.GetCheck()) {
					Sleep(intertrialWaitTime * 1000);
				}

				// start the next trial
				if (m_trialLoopChk.GetCheck()) {
					OnStartTrialBtnClicked();
				}
				break;
			}
		}

	}
}

void CProtocolAppDlg::sendStartRecording()
{
	long currentTrialNumber = m_protocol.currentTrialNumber.load();

	if (m_touchSensorClient.isConnected()) {
		m_touchSensorClient.startRecording(currentTrialNumber);

		// TODO delete old thread if exists
		stopTouchSensorSuccessMonitor.store(false);
		m_touchSensorSuccessMonitorThread = new thread(&CProtocolAppDlg::m_touchSensorSuccessMonitor, this);
	}

}

void CProtocolAppDlg::enableProtocolCtrls(bool enable)
{
	GetDlgItem(IDC_START_PROTOCOL_BTN)->EnableWindow(enable);
	GetDlgItem(IDC_STOP_PROTOCOL_BTN)->EnableWindow(!enable);

	GetDlgItem(IDC_LOAD_CONFIG_BTN)->EnableWindow(enable);
	GetDlgItem(IDC_SAVE_CONFIG_BTN)->EnableWindow(enable);

	GetDlgItem(IDC_ACCELERATION_EDT)->EnableWindow(enable);
	GetDlgItem(IDC_SPEED_EDT)->EnableWindow(enable);
	GetDlgItem(IDC_POSITION_EDT)->EnableWindow(enable);
	GetDlgItem(IDC_MAX_WAIT_EDT_BOX)->EnableWindow(enable);
	GetDlgItem(IDC_INTERTRIAL_WAIT_EDT)->EnableWindow(enable);
}

void CProtocolAppDlg::enableTrialCtrls(bool enable)
{
	GetDlgItem(IDC_START_TRIAL_BTN)->EnableWindow(enable);
	GetDlgItem(IDC_RETREAT_FLUSH_WATER_BTN)->EnableWindow(!enable && m_protocol.params.tstEnReward);
	GetDlgItem(IDC_RETREAT_BTN)->EnableWindow(!enable);
}

void CProtocolAppDlg::enableRewardCtrls(bool enable)
{
	GetDlgItem(IDC_REWARD_TIME_EDT)->EnableWindow(enable && m_protocol.params.tstEnReward);
	GetDlgItem(IDC_FLUSH_WATER_BTN)->EnableWindow(enable && m_protocol.params.tstEnReward);
}

void CProtocolAppDlg::enableCameraServer1Ctrls(bool enable)
{
	GetDlgItem(IDC_IP_EDT1)->EnableWindow(enable);
	GetDlgItem(IDC_PORT_EDT1)->EnableWindow(enable);
	GetDlgItem(IDC_CONNECT_BTN1)->EnableWindow(enable);

	GetDlgItem(IDC_DISCONNECT_BTN1)->EnableWindow(!enable);
}

void CProtocolAppDlg::enableCameraServer2Ctrls(bool enable)
{
	GetDlgItem(IDC_IP_EDT2)->EnableWindow(enable);
	GetDlgItem(IDC_PORT_EDT2)->EnableWindow(enable);
	GetDlgItem(IDC_CONNECT_BTN2)->EnableWindow(enable);

	GetDlgItem(IDC_DISCONNECT_BTN2)->EnableWindow(!enable);
}

void CProtocolAppDlg::enableTouchServerCtrls(bool enable)
{
	GetDlgItem(IDC_TOUCH_SENSOR_IP_EDT)->EnableWindow(enable && m_protocol.params.tstEnTouchSensors);
	GetDlgItem(IDC_TOUCH_SENSOR_PORT_EDT)->EnableWindow(enable && m_protocol.params.tstEnTouchSensors);
	GetDlgItem(IDC_CONNECT_TOUCH_SENSOR_BTN)->EnableWindow(enable && m_protocol.params.tstEnTouchSensors);

	GetDlgItem(IDC_DISCONNECT_TOUCH_SENSOR_BTN)->EnableWindow(!enable && m_protocol.params.tstEnTouchSensors);
}


/*
* maxWaitTime = 5000;
* rewardDuration = 1000;
* nTrialsDesidered = 150;
* acceleration = 2; // proportional level 1-10 (1 - 4000 RPM/S)
* speed = 2;        // proportional level 1-10 (1 - 700 RPM)
* position = 120;     // 1 to 240 mm -> proportional cycles ((-1) to (-105000) CNTs)
* holdingTimeTouchSensor = 500; // msec
* intertrialTime = 2500; //msec
*/

void CProtocolAppDlg::OnSaveProtBtnClicked()
{
	UpdateData(FromControlsToVariables); // updates the frequency of interest
	const TCHAR szFilter[] = _T("Protocol Files (*.pro)|*.pro||");
	CFileDialog dlg(FALSE, _T("Saving a Protocol Config File"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, this);
	if (dlg.DoModal() == IDOK)
	{
		CString sFilePath = dlg.GetPathName();
		std::ofstream os(sFilePath);
		os << m_protocol.params.maxWaitTime << " ";
		os << m_protocol.params.rewardDuration << " ";
		os << m_protocol.params.acceleration << " ";
		os << m_protocol.params.speed << " ";
		os << m_protocol.params.position << endl;
	}
}

void CProtocolAppDlg::OnLoadProtBtnClicked()
{
	const TCHAR szFilter[] = _T("Protocol Files (*.pro)|*.pro||");
	CFileDialog dlg(TRUE, _T("Opening a Protocol Config File"), NULL, NULL, szFilter, this);
	if (dlg.DoModal() == IDOK)
	{
		CString sFilePath = dlg.GetPathName();
		std::ifstream is(sFilePath);
		std::istream_iterator<double> start(is), end;
		std::vector<double> params(start, end);
		if (params.size() != 8)
		{
			AfxMessageBox("Error loading protocol params.Wrong parameters number!");
			return;
		}
		m_protocol.params.maxWaitTime = (long)params[0];
		m_protocol.params.rewardDuration = (long)params[1];
		m_protocol.params.acceleration = (long)params[2];
		m_protocol.params.speed = (long)params[3];
		m_protocol.params.position = (long)params[4];
		UpdateData(FromVariablesToControls);
	}
}

