// ProtocolAppDlg.cpp : implementation file
//

#include "ProtocolAppDlg.h"

constexpr auto ENABLED = true;
constexpr auto DISABLED = false;

// CProtocolAppDlg dialog

CProtocolAppDlg::CProtocolAppDlg(CWnd* pParent /*=nullptr*/) : CDialogEx(IDD_PROTOCOLAPP_DIALOG, pParent), m_stopProtocol(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProtocolAppDlg::DoDataExchange(CDataExchange* pDX)
{
	//OutputDebugString(_T("Start of DoDataExchange\n"));
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_REWARD_TIME_EDT, m_rewardDurationEdtCtrl);
	DDX_Control(pDX, IDC_ACCELERATION_EDT, m_accelerationCtrl);
	DDX_Control(pDX, IDC_SPEED_EDT, m_speedCtrl);
	DDX_Control(pDX, IDC_POSITION_EDT, m_positionCtrl);
	DDX_Control(pDX, IDC_CURRENT_TRIAL_EDT_BOX, m_currentTrialEdtCtrl);
	DDX_Control(pDX, IDC_PHOTORES_FRONT_LBL, m_frontPhotoresistorCtrl);
	DDX_Control(pDX, IDC_PHOTORES_REAR_LBL, m_rearPhotoresistorCtrl);
	
	DDX_Text(pDX, IDC_REWARD_TIME_EDT, m_protocol.params.rewardDuration);
	DDX_Text(pDX, IDC_ACCELERATION_EDT, m_protocol.params.acceleration);
	DDX_Text(pDX, IDC_SPEED_EDT, m_protocol.params.speed);
	DDX_Text(pDX, IDC_POSITION_EDT, m_protocol.params.position);
	DDX_Text(pDX, IDC_MAX_WAIT_EDT_BOX, m_protocol.params.maxWaitTime);

	DDX_Text(pDX, IDC_IP_EDT, m_protocol.params.cs_ip);
	DDX_Text(pDX, IDC_PORT_EDT, m_protocol.params.cs_port);
	DDX_Text(pDX, IDC_FRAMERATE_EDT, m_protocol.params.cs_framerate);

	//OutputDebugString(_T("End of DoDataExchange\n"));
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
	ON_BN_CLICKED(IDC_START_SERVER_BTN, OnStartServerBtnClicked)
	ON_BN_CLICKED(IDC_STOP_SERVER_BTN, OnStopServerBtnClicked)
	ON_BN_CLICKED(IDC_DISCONNECT_CLIENT_BTN, OnDisconnectClientBtnClicked)
	ON_BN_CLICKED(IDC_SEND_CONFIG_BTN, OnSendConfigBtnClicked)
	ON_BN_CLICKED(IDC_SYNC_TIME_BTN, OnSyncTimeBtnClicked)
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

	// set the visibility and defaults for GUI
	if (m_protocol.params.tstEnCameras) ((CButton*)GetDlgItem(IDC_CAMERAS_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.params.tstEnLightSensors) ((CButton*)GetDlgItem(IDC_LIGHT_SENSORS_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.params.tstEnMotors) ((CButton*)GetDlgItem(IDC_MOTORS_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.params.tstEnReward) ((CButton*)GetDlgItem(IDC_REWARD_CHK))->SetCheck(BST_CHECKED);
	if (m_protocol.params.tstEnTouchSensors) ((CButton*)GetDlgItem(IDC_TOUCH_SENSORS_CHK))->SetCheck(BST_CHECKED);

	/////// Control what is enabled and initialized based on debug/testing interface
	enableProtocolCtrls(true);
	enableTrialCtrls(true);
	GetDlgItem(IDC_START_TRIAL_BTN)->EnableWindow(false);
	// Initialize motors

	// Initialize reward
	enableRewardCtrls(true);

	// Initialize cameras
	enableCameraServerCtrls(true);


	// Initialize light sensors

	// Initialize touch sensors
	
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

//void CProtocolAppDlg::OnCancel()
//{
//}

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
	m_stopProtocol.store(false);
	m_startTrial.store(false);
	m_stopTrial.store(false);

	if (m_protocol.params.isNiCardBeingUsed()) {
		m_NIUsb6001card.config();
		if (m_protocol.params.tstEnLightSensors) {
			m_NIUsb6001card.setFrontPhotoresistorMonitor(&m_frontPhotoresistorCtrl);
			m_NIUsb6001card.setRearPhotoresistorMonitor(&m_rearPhotoresistorCtrl);
		}
		m_NIUsb6001card.start();
	}
	
	protocolThread = new thread(&Protocol::run, &m_protocol, &m_stopProtocol, &m_startTrial, &m_stopTrial, &m_NIUsb6001card, &m_currentTrialEdtCtrl);

	enableTrialCtrls(true);
}

void CProtocolAppDlg::OnStopProtocolBtnClicked()
{
	if (m_protocol.params.isNiCardBeingUsed())
		m_NIUsb6001card.stop();
	stopProtocolThread();
	if (m_protocol.params.tstEnLightSensors)
		m_NIUsb6001card.resetPhotoresistorsGuiMonitor();

	// trial controls fully off
	enableTrialCtrls(true);
	GetDlgItem(IDC_START_TRIAL_BTN)->EnableWindow(false);
	// protocol controls and edits are on
	enableProtocolCtrls(ENABLED);
}

void CProtocolAppDlg::OnFlushWaterBtnClicked()
{
	UpdateData(FromControlsToVariables);
	m_NIUsb6001card.reward(m_protocol.params.rewardDuration);
}

void CProtocolAppDlg::OnStartTrialBtnClicked()
{
	enableTrialCtrls(false);

	m_startTrial.store(true);
}

void CProtocolAppDlg::OnRetreatFlushWaterBtnClicked()
{
	UpdateData(FromControlsToVariables);
	m_NIUsb6001card.reward(m_protocol.params.rewardDuration);

	enableTrialCtrls(true);

	// TODO retreat
	m_stopTrial.store(true);
}

void CProtocolAppDlg::OnRetreatBtnClicked()
{
	enableTrialCtrls(true);

	// TODO retreat
	m_stopTrial.store(true);
}

void CProtocolAppDlg::OnStartServerBtnClicked()
{
	// start process with m_cameraServer

	enableCameraServerCtrls(false);
}

void CProtocolAppDlg::OnStopServerBtnClicked()
{
	// disconnect client
	// stop server

	enableCameraServerCtrls(true);
}

void CProtocolAppDlg::OnDisconnectClientBtnClicked()
{
	// TODO
}

void CProtocolAppDlg::OnSendConfigBtnClicked()
{
}

void CProtocolAppDlg::OnSyncTimeBtnClicked()
{
}

void CProtocolAppDlg::stopProtocolThread()
{
	if (protocolThread) {
		m_stopProtocol.store(true);
		protocolThread->join();
		delete protocolThread; protocolThread = nullptr;
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

void CProtocolAppDlg::enableCameraServerCtrls(bool enable)
{
	GetDlgItem(IDC_IP_EDT)->EnableWindow(enable && m_protocol.params.tstEnCameras);
	GetDlgItem(IDC_PORT_EDT)->EnableWindow(enable && m_protocol.params.tstEnCameras);
	GetDlgItem(IDC_START_SERVER_BTN)->EnableWindow(enable && m_protocol.params.tstEnCameras);

	GetDlgItem(IDC_STOP_SERVER_BTN)->EnableWindow(!enable && m_protocol.params.tstEnCameras);
	GetDlgItem(IDC_DISCONNECT_CLIENT_BTN)->EnableWindow(!enable && m_protocol.params.tstEnCameras);

	GetDlgItem(IDC_SEND_CONFIG_BTN)->EnableWindow(!enable && m_protocol.params.tstEnCameras);
	GetDlgItem(IDC_SYNC_TIME_BTN)->EnableWindow(!enable && m_protocol.params.tstEnCameras);
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

