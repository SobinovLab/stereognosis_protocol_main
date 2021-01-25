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

	DDX_Control(pDX, IDC_START_TRIAL_BTN, m_startTrialBtn);
	DDX_Control(pDX, IDC_RETREAT_BTN, m_retreatBtn);
	DDX_Control(pDX, IDC_RETREAT_FLUSH_WATER_BTN, m_retreatFlushBtn);

	DDX_Control(pDX, IDC_REWARD_TIME_EDT, m_rewardDurationEdtCtrl);

	DDX_Control(pDX, IDC_POSITION_EDT, m_positionCtrl);
	DDX_Control(pDX, IDC_CURRENT_TRIAL_EDT_BOX, m_currentTrialEdtCtrl);

	DDX_Control(pDX, IDC_PHOTORES_FRONT_LBL, m_frontPhotoresistorCtrl);
	DDX_Control(pDX, IDC_PHOTORES_REAR_LBL, m_rearPhotoresistorCtrl);

	DDX_Control(pDX, IDC_SERVER_STATUS_EDT1, m_serverStatusCtrl1);
	DDX_Control(pDX, IDC_SERVER_LOG_EDT1, m_serverLogCtrl1);
	DDX_Control(pDX, IDC_SERVER_STATUS_EDT2, m_serverStatusCtrl2);
	DDX_Control(pDX, IDC_SERVER_LOG_EDT3, m_serverLogCtrl2);

	DDX_Control(pDX, IDC_TOUCH_SENSOR_SERVER_LOG_EDT, m_touchServerLogCtrl);

	DDX_Text(pDX, IDC_REWARD_TIME_EDT, m_protocol.params.rewardDuration);
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
	setFontGuiTrialsCounter();  // legacy
	m_protocol.set_photoresistor_monitors(&m_frontPhotoresistorCtrl, &m_rearPhotoresistorCtrl);
	m_serverStatusCtrl1.SetWindowText("Off");
	m_protocol.set_camera1_gui_controls(&m_serverStatusCtrl1, &m_serverLogCtrl1);
	m_serverStatusCtrl2.SetWindowText("Off");
	m_protocol.set_camera2_gui_controls(&m_serverStatusCtrl2, &m_serverLogCtrl2);
	m_protocol.set_pressure_sensors_gui_controls(&m_touchServerLogCtrl);
	m_protocol.set_current_trial_gui_control(&m_currentTrialEdtCtrl);
	m_protocol.set_trial_buttons(&m_startTrialBtn, &m_retreatBtn, &m_retreatFlushBtn);

	// set the visibility of enabled devices on GUI
	if (m_protocol.isLightSensorsOn()) ((CButton*)GetDlgItem(IDC_LIGHT_SENSORS_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.isMotorsOn()) ((CButton*)GetDlgItem(IDC_MOTORS_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.isRewardOn()) ((CButton*)GetDlgItem(IDC_REWARD_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.isEphysOn()) ((CButton*)GetDlgItem(IDC_EPHYS_CHK))->SetCheck(BST_CHECKED);

	/////// Control what is enabled and initialized based on debug/testing interface
	toggleProtocolCtrls(true);
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

/// <summary>
/// If not set, on pressing Enter the program exits
/// </summary>
void CProtocolAppDlg::OnOK()
{
	UpdateData(FromControlsToVariables);
}

/// <summary>
/// Legacy. I guess makes it prettier?
/// </summary>
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
	// don't want to get too many clicks
	toggleProtocolCtrls(false);
	GetDlgItem(IDC_STOP_PROTOCOL_BTN)->EnableWindow(false);

	// in case any parameters were changed
	UpdateData(FromControlsToVariables);
	protocolThread = new thread(&Protocol::run, &m_protocol);

	// enables stopping of the protocol
	GetDlgItem(IDC_STOP_PROTOCOL_BTN)->EnableWindow(true);
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
	m_protocol.deservesReward = true;

	stopTrial();
}

/// <summary>
/// Async stop trial
/// </summary>
void CProtocolAppDlg::OnRetreatBtnClicked()
{
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

	// TODO return error if not connected and not change state?
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
		protocolThread->join();
		delete protocolThread; protocolThread = nullptr;
	}
}

void CProtocolAppDlg::stopTrial()
{
	// stop trial
	m_protocol.stopTrial.store(true);
}

void CProtocolAppDlg::toggleProtocolCtrls(bool stopped)
{
	GetDlgItem(IDC_START_PROTOCOL_BTN)->EnableWindow(stopped);
	GetDlgItem(IDC_STOP_PROTOCOL_BTN)->EnableWindow(!stopped);

	GetDlgItem(IDC_POSITION_EDT)->EnableWindow(stopped);
	GetDlgItem(IDC_MAX_WAIT_EDT_BOX)->EnableWindow(stopped);
	GetDlgItem(IDC_INTERTRIAL_WAIT_EDT)->EnableWindow(stopped);
}

void CProtocolAppDlg::enableRewardCtrls(bool enable)
{
	GetDlgItem(IDC_REWARD_TIME_EDT)->EnableWindow(enable && m_protocol.isRewardOn());
	GetDlgItem(IDC_FLUSH_WATER_BTN)->EnableWindow(enable && m_protocol.isRewardOn());
}

void CProtocolAppDlg::toggleCameraServer1Ctrls(bool disconnected)
{
	GetDlgItem(IDC_IP_EDT1)->EnableWindow(disconnected);
	GetDlgItem(IDC_PORT_EDT1)->EnableWindow(disconnected);
	GetDlgItem(IDC_CONNECT_BTN1)->EnableWindow(disconnected);

	GetDlgItem(IDC_DISCONNECT_BTN1)->EnableWindow(!disconnected);
}

void CProtocolAppDlg::toggleCameraServer2Ctrls(bool disconnected)
{
	GetDlgItem(IDC_IP_EDT2)->EnableWindow(disconnected);
	GetDlgItem(IDC_PORT_EDT2)->EnableWindow(disconnected);
	GetDlgItem(IDC_CONNECT_BTN2)->EnableWindow(disconnected);

	GetDlgItem(IDC_DISCONNECT_BTN2)->EnableWindow(!disconnected);
}

void CProtocolAppDlg::toggleTouchServerCtrls(bool disconnected)
{
	GetDlgItem(IDC_TOUCH_SENSOR_IP_EDT)->EnableWindow(disconnected);
	GetDlgItem(IDC_TOUCH_SENSOR_PORT_EDT)->EnableWindow(disconnected);
	GetDlgItem(IDC_CONNECT_TOUCH_SENSOR_BTN)->EnableWindow(disconnected);

	GetDlgItem(IDC_DISCONNECT_TOUCH_SENSOR_BTN)->EnableWindow(!disconnected);
}
