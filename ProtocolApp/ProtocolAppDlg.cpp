// ProtocolAppDlg.cpp : implementation file
//

#include "ProtocolAppDlg.h"

using namespace std;

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

	// CONTROL LINKS
	// protocol
	DDX_Control(pDX, IDC_PHOTORES_FRONT_LBL, m_frontPhotoresistorCtrl);
	DDX_Control(pDX, IDC_PHOTORES_REAR_LBL, m_rearPhotoresistorCtrl);

	// trial
	DDX_Control(pDX, IDC_START_TRIAL_BTN, m_startTrialBtn);
	DDX_Control(pDX, IDC_RETREAT_BTN, m_retreatBtn);
	DDX_Control(pDX, IDC_RETREAT_FLUSH_WATER_BTN, m_retreatFlushBtn);
	DDX_Control(pDX, IDC_LOOP_CHK, m_loopChk);
	DDX_Control(pDX, IDC_USE_FRONT_LIGHT_SENSOR_CHK, m_useFrontLightSensorChk);
	DDX_Control(pDX, IDC_USE_REAR_LIGHT_SENSOR_CHK, m_useRearLightSensorChk);

	DDX_Control(pDX, IDC_TRIAL_STATUS, m_trialStatus);

	// pressure sensors
	DDX_Control(pDX, IDC_TOUCH_SENSOR_SERVER_LOG_EDT, m_touchServerLogCtrl);

	// cameras
	DDX_Control(pDX, IDC_SERVER_LOG_EDT1, m_serverLogCtrl1);
	DDX_Control(pDX, IDC_SERVER_LOG_EDT2, m_serverLogCtrl2);

	// LEDs
	DDX_Control(pDX, IDC_LEDS_EARLY_TARGET_FORCE_LIGHT_CHK, m_ledsEarlyTargetForceLightChk);

	// VARIABLE LINKS
	// protocol
	DDX_Text(pDX, IDC_MAX_WAIT_EDT_BOX, m_protocol.params.maxWaitTime);
	DDX_Text(pDX, IDC_INTERTRIAL_WAIT_EDT, m_protocol.params.intertrialWaitTime);
	DDX_Text(pDX, IDC_SESSION_FILE_EDT, m_protocol.params.session_filename);
	DDX_Text(pDX, IDC_SESSION_LOG_FILE_EDT, m_protocol.params.session_log_filename);
	DDX_Text(pDX, IDC_REWARD_TIME_EDT, m_protocol.params.rewardDuration);

	// trial
	DDX_Text(pDX, IDC_CURRENT_TRIAL_EDT, m_protocol.params.trial_number);
	DDX_Text(pDX, IDC_TOTAL_TRIALS_EDT, m_protocol.params.total_trials);

	DDX_Text(pDX, IDC_POS_TRANSLATION_X_EDT, m_protocol.params.pos_translation_x);
	DDX_Text(pDX, IDC_POS_TILT_EDT, m_protocol.params.pos_tilt);
	DDX_Text(pDX, IDC_POS_APERTURE_EDT, m_protocol.params.pos_aperture);

	// pressure sensors
	DDX_Text(pDX, IDC_TOUCH_SENSOR_IP_EDT, m_protocol.params.tss_ip);
	DDX_Text(pDX, IDC_TOUCH_SENSOR_PORT_EDT, m_protocol.params.tss_port);

	// success conditions
	DDX_Text(pDX, IDC_PS_TARGET_FORCE_EDT, m_protocol.params.targetForce);
	DDX_Text(pDX, IDC_PS_REL_RANGE_MIN_EDT, m_protocol.params.targetForceRelRangeMin);
	DDX_Text(pDX, IDC_PS_REL_RANGE_MAX_EDT, m_protocol.params.targetForceRelRangeMax);
	DDX_Text(pDX, IDC_PS_TOTAL_FORCE_MIN_EDT, m_protocol.params.targetForceTotalMinThreshold);
	DDX_Text(pDX, IDC_PS_TOTAL_FORCE_MAX_EDT, m_protocol.params.targetForceTotalMax);
	DDX_Text(pDX, IDC_PS_HOLD_PERIOD_EDT, m_protocol.params.thresholdPeriod);
	DDX_Text(pDX, IDC_PS_MIN_PARTIAL_FORCE_EDT, m_protocol.params.thresholdForceEachProportion);
	DDX_Text(pDX, IDC_PS_MINIMAL_TOUCH_FORCE_EDT, m_protocol.params.minimalTouchForce);

	// cameras
	DDX_Text(pDX, IDC_IP_EDT1, m_protocol.params.cs_ip1);
	DDX_Text(pDX, IDC_PORT_EDT1, m_protocol.params.cs_port1);
	DDX_Text(pDX, IDC_IP_EDT2, m_protocol.params.cs_ip2);
	DDX_Text(pDX, IDC_PORT_EDT2, m_protocol.params.cs_port2);

	// cameras_config
	DDX_Text(pDX, IDC_FRAMERATE_EDT, m_protocol.params.cs_framerate);
	DDX_Text(pDX, IDC_RECORDING_PERIOD_EDT, m_protocol.params.cs_recordingPeriod);
	DDX_Text(pDX, IDC_REF_SERIAL_EDT, m_protocol.params.cs_refSerial);
	DDX_Text(pDX, IDC_GAIN_EDT, m_protocol.params.cs_gain);
	DDX_Text(pDX, IDC_EXPOSURE_EDT, m_protocol.params.cs_exposure);
	DDX_Text(pDX, IDC_CAPTURE_N_IMAGES_EDT, m_protocol.params.cs_capture_n_frames);

}

BEGIN_MESSAGE_MAP(CProtocolAppDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_FLUSH_WATER_BTN, &OnFlushWaterBtnClicked)
	ON_BN_CLICKED(IDC_STOP_PROTOCOL_BTN, &OnStopProtocolBtnClicked)
	ON_BN_CLICKED(IDC_START_PROTOCOL_BTN, &OnStartProtocolBtnClicked)
	ON_BN_CLICKED(IDC_START_TRIAL_BTN, &OnStartTrialBtnClicked)
	ON_BN_CLICKED(IDC_RETREAT_FLUSH_WATER_BTN, &OnRetreatFlushWaterBtnClicked)
	ON_BN_CLICKED(IDC_RETREAT_BTN, &OnRetreatBtnClicked)
	ON_BN_CLICKED(IDC_CONNECT_BTN1, &OnConnect1BtnClicked)
	ON_BN_CLICKED(IDC_DISCONNECT_BTN1, &OnDisconnect1BtnClicked)
	ON_BN_CLICKED(IDC_CONNECT_BTN2, &OnConnect2BtnClicked)
	ON_BN_CLICKED(IDC_DISCONNECT_BTN2, &OnDisconnect2BtnClicked)
	ON_BN_CLICKED(IDC_SEND_CONFIG_BTN, &OnSendConfigBtnClicked)
	ON_BN_CLICKED(IDC_CAPTURE_SINGLE_FRAME_BTN, &OnCaptureSingleFrameBtnClicked)
	ON_BN_CLICKED(IDC_CONNECT_TOUCH_SENSOR_BTN, &OnConnectTouchSensorBtnClicked)
	ON_BN_CLICKED(IDC_DISCONNECT_TOUCH_SENSOR_BTN, &OnDisconnectTouchSensorBtnClicked)
	ON_BN_CLICKED(IDC_HOME_MOTORS_BTN, &OnBnClickedHomeMotorsBtn)
	ON_BN_CLICKED(IDC_LOOP_CHK, &CProtocolAppDlg::OnBnClickedLoopChk)
	ON_BN_CLICKED(IDC_USE_FRONT_LIGHT_SENSOR_CHK, &CProtocolAppDlg::OnBnClickedUseFrontLightSensorChk)
	ON_BN_CLICKED(IDC_USE_REAR_LIGHT_SENSOR_CHK, &CProtocolAppDlg::OnBnClickedUseRearLightSensorChk)
	ON_BN_CLICKED(IDC_STOP_MOTORS_BTN, &CProtocolAppDlg::OnBnClickedStopMotorsBtn)
	ON_BN_CLICKED(IDC_NEUTRAL_POSITION_BTN, &CProtocolAppDlg::OnBnClickedNeutralPositionBtn)
	ON_BN_CLICKED(IDC_LEDS_EARLY_TARGET_FORCE_LIGHT_CHK, &CProtocolAppDlg::OnBnClickedLedsEarlyTargetForceLightChk)
    ON_EN_CHANGE(IDC_SESSION_FILE_EDT, &CProtocolAppDlg::OnEnChangeSessionFileEdt)
    ON_BN_CLICKED(IDC_MOTORS_CHK, &CProtocolAppDlg::OnBnClickedMotorsChk)
   

    ON_BN_CLICKED(IDC_LEFT_MAIN, &CProtocolAppDlg::OnBnClickedLeftMain)
    ON_BN_CLICKED(IDC_LEFT_SECOND, &CProtocolAppDlg::OnBnClickedLeftSecond)
    ON_BN_CLICKED(IDC_RIGHT_MAIN, &CProtocolAppDlg::OnBnClickedRightMain)
    ON_BN_CLICKED(IDC_RIGHT_SECOND, &CProtocolAppDlg::OnBnClickedRightSecond)
    ON_BN_CLICKED(IDC_SPLIT_REWARD, &CProtocolAppDlg::OnBnClickedSplitReward)
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
	setFontGuiTrialsCounter();  // legacy
	m_protocol.mainWindow = this;

	m_protocol.set_photoresistor_monitors(&m_frontPhotoresistorCtrl, &m_rearPhotoresistorCtrl, );
	m_protocol.set_camera1_gui_controls(&m_serverLogCtrl1);
	m_protocol.set_camera2_gui_controls(&m_serverLogCtrl2);
	m_protocol.set_pressure_sensors_gui_controls(&m_touchServerLogCtrl);

	m_protocol.set_trial_buttons(&m_startTrialBtn, &m_retreatBtn, &m_retreatFlushBtn);
	m_protocol.m_trialStatus = &m_trialStatus;
	m_protocol.trialStateGuiUpdate();

	((CButton*)GetDlgItem(IDC_LOOP_CHK))->SetCheck(BST_CHECKED);
	m_protocol.loopChk = &m_loopChk;
	// there is a better way to synchronize the two variables, you are welcome to do it
	m_useFrontLightSensorChk.SetCheck(BST_UNCHECKED);
	m_useRearLightSensorChk.SetCheck(BST_UNCHECKED);
	m_protocol.use_front_light_sensor = false;
	m_protocol.use_rear_light_sensor = false;

	// set the visibility of enabled devices on GUI
	if (m_protocol.isLightSensorsOn()) ((CButton*)GetDlgItem(IDC_LIGHT_SENSORS_CHK))->SetCheck(BST_CHECKED);
	//if (m_protocol.isMotorsOn()) ((CButton*)GetDlgItem(IDC_MOTORS_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.isRewardOn()) ((CButton*)GetDlgItem(IDC_REWARD_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.isEphysOn()) ((CButton*)GetDlgItem(IDC_EPHYS_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.isLedsOn()) ((CButton*)GetDlgItem(IDC_LEDS_CHK))->SetCheck(BST_CHECKED);

	/////// Control what is enabled and initialized
	// protocol
	toggleProtocolCtrls(true);
	//GetDlgItem(IDC_HOME_MOTORS_BTN)->EnableWindow(m_protocol.isMotorsOn());
	GetDlgItem(IDC_NEUTRAL_POSITION_BTN)->EnableWindow(m_protocol.isMotorsOn());
	GetDlgItem(IDC_STOP_MOTORS_BTN)->EnableWindow(m_protocol.isMotorsOn());

	// trial
	m_startTrialBtn.EnableWindow(false);
	m_retreatBtn.EnableWindow(false);
	m_retreatFlushBtn.EnableWindow(false);

	// reward
	enableRewardCtrls(true);

	// cameras
	toggleCameraServer1Ctrls(true);
	toggleCameraServer2Ctrls(true);

	// touch sensor
	toggleTouchServerCtrls(true);

	// leds
	m_ledsEarlyTargetForceLightChk.EnableWindow(m_protocol.isLedsOn());
	if (m_protocol.params.leds_early_target_force_lightup) 
		m_ledsEarlyTargetForceLightChk.SetCheck(BST_CHECKED);
	else
		m_ledsEarlyTargetForceLightChk.SetCheck(BST_UNCHECKED);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CProtocolAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	UINT command = (nID & 0xFFF0);

	if (command == SC_CLOSE) {
		if (GetDlgItem(IDC_STOP_PROTOCOL_BTN)->IsWindowEnabled())
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

/// <summary>
/// If not set, on pressing Enter the program exits
/// </summary>
void CProtocolAppDlg::OnOK()
{
	UpdateData(FromControlsToVariables);
}

BOOL CProtocolAppDlg::PreTranslateMessage(MSG* pMsg)
{
	// bad practice, but fuck mfc
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case 's':
		case 'S':
			OnKeyPress_S();
		case 't':
		case 'T':
			OnKeyPress_T();
		case 'w':
		case 'W':
			OnKeyPress_W();
		case 'f':
		case 'F':
			OnKeyPress_F();
		default:
			break;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

void CProtocolAppDlg::OnKeyPress_S()
{
	if (GetDlgItem(IDC_START_TRIAL_BTN)->IsWindowEnabled())
		OnStartTrialBtnClicked();
}

void CProtocolAppDlg::OnKeyPress_T()
{
	if (GetDlgItem(IDC_RETREAT_BTN)->IsWindowEnabled())
		OnRetreatBtnClicked();
}

void CProtocolAppDlg::OnKeyPress_W()
{
	if (GetDlgItem(IDC_RETREAT_FLUSH_WATER_BTN)->IsWindowEnabled())
		OnRetreatFlushWaterBtnClicked();
}

void CProtocolAppDlg::OnKeyPress_F()
{
	if (GetDlgItem(IDC_FLUSH_WATER_BTN)->IsWindowEnabled())
		OnFlushWaterBtnClicked();
}

/// <summary>
/// Legacy. I guess makes it prettier?
/// </summary>
void CProtocolAppDlg::setFontGuiTrialsCounter()
{
	CFont* cEditControlFont = new CFont();
	cEditControlFont->CreateFont(30, 0, 0, 0, FW_HEAVY, true, false, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(FONT_TYPE));
	((CEdit*)GetDlgItem(IDC_CURRENT_TRIAL_EDT))->SetFont(cEditControlFont);
	((CEdit*)GetDlgItem(IDC_TOTAL_TRIALS_EDT))->SetFont(cEditControlFont);
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CProtocolAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CProtocolAppDlg::OnStartProtocolBtnClicked()
{
	if (m_protocol.getCurrentState() == ProtocolState::shutdown) {
		// don't want to get too many clicks
		toggleProtocolCtrls(false);
		GetDlgItem(IDC_STOP_PROTOCOL_BTN)->EnableWindow(false);

		// check if homed
		if (m_protocol.isMotorsOn()) {
			if (!m_protocol.were_motors_homed())
				AfxMessageBox("Motors were not homed. Please home them prior to starting the trial.");
		}

        //check if arm homed
        if (!m_protocol.armHomed)
        {
            AfxMessageBox("The arm was not homed, make sure you click the calibrate arm buton");
            GetDlgItem(IDC_STOP_PROTOCOL_BTN)->EnableWindow(true);
            return;
        }

		// in case any parameters were changed
		UpdateData(FromControlsToVariables);
		protocolThread = new thread(&Protocol::run, &m_protocol);

		// enables stopping of the protocol
		GetDlgItem(IDC_STOP_PROTOCOL_BTN)->EnableWindow(true);
	}
	else {
		AfxMessageBox("Protocol was not shutdown correctly or is in process of shutting down. Wait or restart.");
	}
}

void CProtocolAppDlg::OnStopProtocolBtnClicked()
{
	// don't want too many clicks
	GetDlgItem(IDC_STOP_PROTOCOL_BTN)->EnableWindow(false);

	// this will wait until protocol handles the trial end and finishes the run thread
	stopProtocolThread();

	// protocol controls and edits are on
	toggleProtocolCtrls(true);
}

// TODO: possible async problem accessing the params from GUI thread and Protocol
void CProtocolAppDlg::OnFlushWaterBtnClicked()
{
	UpdateData(FromControlsToVariables);
	m_protocol.reward();
}

/// <summary>
/// Async signal to Protocol thread
/// </summary>
void CProtocolAppDlg::OnStartTrialBtnClicked()
{
	m_protocol.startTrial.store(true);
}

/// <summary>
/// Forces the reward regardless of performance. Async stop trial
/// </summary>
void CProtocolAppDlg::OnRetreatFlushWaterBtnClicked()
{
	logInfo("Retreat flush water button clicked.");
	m_protocol.deservesReward = true;

	stopTrial();
}

/// <summary>
/// Async stop trial
/// </summary>
void CProtocolAppDlg::OnRetreatBtnClicked()
{
	logInfo("Retreat button clicked.");
	stopTrial();
}

void CProtocolAppDlg::OnConnect1BtnClicked()
{
	UpdateData(FromControlsToVariables);

	m_protocol.connect_camera_client1();

	toggleCameraServer1Ctrls(false);
}

void CProtocolAppDlg::OnDisconnect1BtnClicked()
{
	m_protocol.disconnect_camera_client1();

	toggleCameraServer1Ctrls(true);
}

void CProtocolAppDlg::OnConnect2BtnClicked()
{
	UpdateData(FromControlsToVariables);

	m_protocol.connect_camera_client2();

	toggleCameraServer2Ctrls(false);
}

void CProtocolAppDlg::OnDisconnect2BtnClicked()
{
	m_protocol.disconnect_camera_client2();

	toggleCameraServer2Ctrls(true);
}

void CProtocolAppDlg::OnSendConfigBtnClicked()
{
	UpdateData(FromControlsToVariables);

	m_protocol.send_config_to_cameras();
}

void CProtocolAppDlg::OnCaptureSingleFrameBtnClicked()
{
	UpdateData(FromControlsToVariables);

	int res = m_protocol.capture_single_frame();
	if (res) {
		string buf = "Error encountered. Code: " + to_string(res) + ". Consult the log and camera computer.";
		AfxMessageBox(buf.c_str());
	}
}

void CProtocolAppDlg::OnConnectTouchSensorBtnClicked()
{
	UpdateData(FromControlsToVariables);

	// return error if not connected and not change state?
	// consider implications for the sensor server disappearing and reappearing, might be worse
	// same in camera servers
	m_protocol.connect_pressure_sensors();

	toggleTouchServerCtrls(false);
}

void CProtocolAppDlg::OnDisconnectTouchSensorBtnClicked()
{
	m_protocol.disconnect_pressure_sensors();

	toggleTouchServerCtrls(true);
}

/// <summary>
/// Locks the current thread until protocolThread with Protocol::run ends
/// </summary>
void CProtocolAppDlg::stopProtocolThread()
{
	if (protocolThread) {
		m_protocol.stopProtocol.store(true);
		stopTrial();
		// joining the thread leads to race for the interface access which is locked in this thread.
		// that's why start protocol checks for the protocol being shutdown
		//protocolThread->join();
		//delete protocolThread; protocolThread = nullptr;
	}
}

void CProtocolAppDlg::stopTrial()
{
	// interrupt the motors if they were running
	// runs before the stopTrial because in the opposite order stop motors might interrupt 
	// the retreat
	m_protocol.stop_motors();
	// stop trial
	m_protocol.stopTrial.store(true);
}

void CProtocolAppDlg::homingMotorAction()
{
	auto state = m_protocol.getCurrentState();
	if (state == ProtocolState::shutdown ||
		state == ProtocolState::trialReady) {
		auto answ = m_protocol.home_motors();
		string buf;
		if (answ < 0) {
			buf = ("Error performing move command. Code: " + to_string((int)answ) + "." +
				" Message: " + checkKinovaErrCode(answ));
			AfxMessageBox(buf.c_str());
		}
	}
	else
		AfxMessageBox("Cannot home motors while the trials are running or initializing.");

	AfxMessageBox("REMOVE THE CALIBRATION FIXATOR AND CLICK OK");

	motorActionInProgress = false;
	GetDlgItem(IDC_HOME_MOTORS_BTN)->EnableWindow(true);
}

void CProtocolAppDlg::neutralPositionMotorAction()
{
	auto state = m_protocol.getCurrentState();
	if (state == ProtocolState::shutdown ||
		state == ProtocolState::trialReady) {
		auto answ = m_protocol.motors_neutral_position();
		string buf;
		if (answ < 0) {
			buf = ("Error performing move command. Code: " + to_string((int)answ) + "." +
				" Message: " + checkKinovaErrCode(answ));
			AfxMessageBox(buf.c_str());
		}
	}
	else
		AfxMessageBox("Cannot neutral position motors while the trials are running or initializing.");

	motorActionInProgress = false;
	GetDlgItem(IDC_NEUTRAL_POSITION_BTN)->EnableWindow(true);
}

void CProtocolAppDlg::toggleProtocolCtrls(bool stopped)
{
	// buttons
	GetDlgItem(IDC_START_PROTOCOL_BTN)->EnableWindow(stopped);
	GetDlgItem(IDC_STOP_PROTOCOL_BTN)->EnableWindow(!stopped);

	// edits that define a session
	((CEdit*)GetDlgItem(IDC_MAX_WAIT_EDT_BOX))->SetReadOnly(!stopped);
	((CEdit*)GetDlgItem(IDC_INTERTRIAL_WAIT_EDT))->SetReadOnly(!stopped);
	((CEdit*)GetDlgItem(IDC_SESSION_FILE_EDT))->SetReadOnly(!stopped);
	((CEdit*)GetDlgItem(IDC_SESSION_LOG_FILE_EDT))->SetReadOnly(!stopped);

	// reward duration
	// access conflict possibility should be low
	//((CEdit*)GetDlgItem(IDC_REWARD_TIME_EDT))->SetReadOnly(!(stopped && m_protocol.isRewardOn()));
}

void CProtocolAppDlg::enableRewardCtrls(bool enable)
{
	// button
	GetDlgItem(IDC_FLUSH_WATER_BTN)->EnableWindow(enable && m_protocol.isRewardOn());
}

void CProtocolAppDlg::toggleCameraServer1Ctrls(bool disconnected)
{
	// edits
	((CEdit*)GetDlgItem(IDC_IP_EDT1))->SetReadOnly(!disconnected);
	((CEdit*)GetDlgItem(IDC_PORT_EDT1))->SetReadOnly(!disconnected);

	// buttons
	GetDlgItem(IDC_CONNECT_BTN1)->EnableWindow(disconnected);

	GetDlgItem(IDC_DISCONNECT_BTN1)->EnableWindow(!disconnected);
}

void CProtocolAppDlg::toggleCameraServer2Ctrls(bool disconnected)
{
	// edits
	((CEdit*)GetDlgItem(IDC_IP_EDT2))->SetReadOnly(!disconnected);
	((CEdit*)GetDlgItem(IDC_PORT_EDT2))->SetReadOnly(!disconnected);

	// buttons
	GetDlgItem(IDC_CONNECT_BTN2)->EnableWindow(disconnected);

	GetDlgItem(IDC_DISCONNECT_BTN2)->EnableWindow(!disconnected);
}

void CProtocolAppDlg::toggleTouchServerCtrls(bool disconnected)
{
	// edits
	((CEdit*)GetDlgItem(IDC_TOUCH_SENSOR_IP_EDT))->SetReadOnly(!disconnected);
	((CEdit*)GetDlgItem(IDC_TOUCH_SENSOR_PORT_EDT))->SetReadOnly(!disconnected);

	// buttons
	GetDlgItem(IDC_CONNECT_TOUCH_SENSOR_BTN)->EnableWindow(disconnected);

	GetDlgItem(IDC_DISCONNECT_TOUCH_SENSOR_BTN)->EnableWindow(!disconnected);
}


void CProtocolAppDlg::OnBnClickedHomeMotorsBtn()
{
    if(((CButton*)GetDlgItem(IDC_MOTORS_CHK))->GetCheck() == BST_UNCHECKED)
    {
        AfxMessageBox("Initialize the motors by clicking the check button then retry this");
        return;
    }
    int ret = m_protocol.armClient->armReady();
    if(ret < 0)
    {
        AfxMessageBox("SOME SHIT IS FUCKED, SHOULD HAVE HOMED");
    }
    else
    {
        AfxMessageBox("Good Job, we are ready to go");
        m_protocol.armHomed = true;
        GetDlgItem(IDC_HOME_MOTORS_BTN)->EnableWindow(false);
    }
    return;
	//*DEPRECATED* spawn a thread so it can be interrupted
	//motorActionThread = new thread(&CProtocolAppDlg::homingMotorAction, this);
    
}


void CProtocolAppDlg::OnBnClickedLoopChk()
{
	if (m_loopChk.GetCheck())
		m_protocol.loopAutomatically = true;
	else
		m_protocol.loopAutomatically = false;
}


void CProtocolAppDlg::OnBnClickedUseFrontLightSensorChk()
{
	if (m_useFrontLightSensorChk.GetCheck())
		m_protocol.use_front_light_sensor = true;
	else
		m_protocol.use_front_light_sensor = false;
}


void CProtocolAppDlg::OnBnClickedUseRearLightSensorChk()
{
	if (m_useRearLightSensorChk.GetCheck())
		m_protocol.use_rear_light_sensor = true;
	else
		m_protocol.use_rear_light_sensor = false;
}


void CProtocolAppDlg::OnBnClickedStopMotorsBtn()
{
	m_protocol.stop_motors();
}


void CProtocolAppDlg::OnBnClickedNeutralPositionBtn()
{
	GetDlgItem(IDC_NEUTRAL_POSITION_BTN)->EnableWindow(false);

	if (motorActionInProgress) {
		AfxMessageBox("Error state. Cannot start a motor action.");
		return;
	}
	motorActionInProgress = true;

	// spawn a thread so it can be interrupted
	motorActionThread = new thread(&CProtocolAppDlg::neutralPositionMotorAction, this);
}


void CProtocolAppDlg::OnBnClickedLedsEarlyTargetForceLightChk()
{
	if (m_ledsEarlyTargetForceLightChk.GetCheck())
		m_protocol.params.leds_early_target_force_lightup = true;
	else
		m_protocol.params.leds_early_target_force_lightup = false;
}


void CProtocolAppDlg::OnEnChangeSessionFileEdt()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialogEx::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}


void CProtocolAppDlg::OnBnClickedMotorsChk()
{
    int butState = ((CButton*)GetDlgItem(IDC_MOTORS_CHK))->GetCheck();
    if(butState == BST_UNCHECKED)
    {
        m_protocol.armClient->connect();
        ((CButton*)GetDlgItem(IDC_MOTORS_CHK))->SetCheck(BST_CHECKED);
        GetDlgItem(IDC_HOME_MOTORS_BTN)->EnableWindow(true);
    }
    else if(butState == BST_CHECKED)
    {
        m_protocol.armClient->disconnect();
        m_protocol.armHomed = false;
		GetDlgItem(IDC_HOME_MOTORS_BTN)->EnableWindow(false);
        ((CButton*)GetDlgItem(IDC_MOTORS_CHK))->SetCheck(BST_UNCHECKED);
    }
    else
    {
        AfxMessageBox("UHHH some weird third state has occured, try again maybe?");
    }
}





void CProtocolAppDlg::OnBnClickedLeftMain()
{
    cout<<"Clicked the left main button\n";
    CButton* tmpButton = (CButton *)GetDlgItem(IDC_LEFT_MAIN);
    if(tmpButton->GetCheck())
    {
        cout<<"Left Main Button is checked\n";
    }
    else
    {
        cout<<"Left Main Button is unchecked\n";
    }
}


void CProtocolAppDlg::OnBnClickedLeftSecond()
{
    cout<<"Clicked the left second button\n";
    CButton* tmpButton = (CButton *)GetDlgItem(IDC_LEFT_SECOND);
    if(tmpButton->GetCheck())
    {
        cout<<"Left Second Button is checked\n";
    }
    else
    {
        cout<<"Left Second Button is unchecked\n";
    }
}


void CProtocolAppDlg::OnBnClickedRightMain()
{
    cout<<"Clicked the right main button\n";
    CButton* tmpButton = (CButton *)GetDlgItem(IDC_RIGHT_MAIN);
    if(tmpButton->GetCheck())
    {
        cout<<"Right Main Button is checked\n";
    }
    else
    {
        cout<<"Right Main Button is unchecked\n";
    }
}


void CProtocolAppDlg::OnBnClickedRightSecond()
{
    cout<<"Clicked on the right second button\n";
    CButton* tmpButton = (CButton *)GetDlgItem(IDC_RIGHT_SECOND);
    if(tmpButton->GetCheck())
    {
        cout<<"Right second Button is checked\n";
    }
    else
    {
        cout<<"Right second Button is unchecked\n";
    }
}


void CProtocolAppDlg::OnBnClickedSplitReward()
{
    cout << "Clicked on the reward on return button\n";
    CButton* tmpButton = (CButton *)GetDlgItem(IDC_SPLIT_REWARD);
    if(tmpButton->GetCheck())
    {
        cout<<"Reward on return Button is checked\n";
        m_protocol.reward_on_return.store(true);
    }
    else
    {
        cout<<"Reward on return button is unchecked\n";
        m_protocol.reward_on_return.store(false);
    }
}
