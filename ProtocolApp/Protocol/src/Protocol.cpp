#include "Protocol.h"

using namespace std;

constexpr auto DATA_FOLDER = "./data";
constexpr auto TRIAL_NUM_STR = "Trial n.";
constexpr auto TRIAL_ABORT_STR = " Aborted";
constexpr auto PRECISION = "%03d";


Protocol::Protocol()
{
	this->stopProtocol.store(false);
	this->startTrial.store(false);
	this->stopTrial.store(false);

	this->deservesReward.store(false);
	this->loopAutomatically.store(true);

	this->protocolState.store(ProtocolState::shutdown);

	// based on params, try to test if things can be enabled and then change the variables
	// TODO: after motor integration
	if (params.tstEnMotors) {
		// TODO check if motors are connected and can be found
	}
	if (params.tstEnLightSensors) {
		// TODO: not using sensors right now at all
	}
	if (params.tstEnEphys) {
		// TODO: check if an NI impulse can be sent, if not set to false
	}
	if (params.tstEnReward) {
		// 
	}
}

Protocol::~Protocol()
{
	// if incorrect termination
	releaseDevices();
}

ProtocolState Protocol::getCurrentState()
{
	return protocolState.load();
}

void Protocol::reward()
{
	reward(params.rewardDuration);
}

void Protocol::reward(long duration)
{
	if (params.tstEnReward)
		m_NIUsb6001card.reward(duration);
}

void Protocol::connect_camera_client1()
{
	if (m_cameraClient1.isConnected()) {
		// TODO warning
	}
	else {
		m_cameraClient1.server_ip = params.cs_ip1;
		m_cameraClient1.port = params.cs_port1;

		m_cameraClient1.connect_f();
	}
}

void Protocol::connect_camera_client2()
{
	if (m_cameraClient2.isConnected()) {
		// TODO warning
	}
	else {
		m_cameraClient2.server_ip = params.cs_ip2;
		m_cameraClient2.port = params.cs_port2;

		m_cameraClient2.connect_f();
	}
}

void Protocol::disconnect_camera_client1()
{
	m_cameraClient1.disconnect_f();
}

void Protocol::disconnect_camera_client2()
{
	m_cameraClient2.disconnect_f();
}

void Protocol::send_config_to_cameras()
{
	if (m_cameraClient1.isConnected()) {
		m_cameraClient1.sendFramerate(params.cs_framerate);
		m_cameraClient1.sendRecordingPeriod(params.cs_recordingPeriod);
		m_cameraClient1.sendReferenceCamera(params.cs_refSerial);
		m_cameraClient1.sendGain(params.cs_gain);
		m_cameraClient1.sendExposure(params.cs_exposure);
	}
	if (m_cameraClient2.isConnected()) {
		m_cameraClient2.sendFramerate(params.cs_framerate);
		m_cameraClient2.sendRecordingPeriod(params.cs_recordingPeriod);
		m_cameraClient2.sendReferenceCamera(params.cs_refSerial);
		m_cameraClient2.sendGain(params.cs_gain);
		m_cameraClient2.sendExposure(params.cs_exposure);
	}
}

int Protocol::capture_single_frame()
{
	send_config_to_cameras();

	int success = 0;
	int answ = 0;
	string buf;

	if (m_cameraClient1.isConnected())
		if (!m_cameraClient1.captureSingleFrame(&success))
			answ = 1;  // to not mistake with the errors from success
	if (m_cameraClient2.isConnected())
		if (!m_cameraClient2.captureSingleFrame(&success))
			answ = 1;
	if (answ) {
		buf = "Server error during capture single frame. Code " + to_string(answ);
		logError(buf.c_str());
	}
	if (success) {
		buf = "Error during capture single frame. Code " + to_string(success);
		logError(buf.c_str());
		answ = success;
	}
	return answ;
}

void Protocol::prepare_camera_recording()
{
	if (m_cameraClient1.isConnected()) {
		m_cameraClient1.prepareRecording();
	}
	if (m_cameraClient2.isConnected()) {
		m_cameraClient2.prepareRecording();
	}
}

int Protocol::start_camera_recording()
{
	return start_camera_recording(currentTrialNumber.load());
}

int Protocol::start_camera_recording(long trial_number)
{
	// NB config is sent separately in the main loop

	int success = 0;
	int answ = 0;  // cameras not connected is not an error
	string buf;

	if (m_cameraClient1.isConnected()) {
		if (!m_cameraClient1.startRecording(trial_number, &success))
			answ = 1;  // to not mistake with the errors from success
	}
	if (m_cameraClient2.isConnected()) {
		if (!m_cameraClient2.startRecording(trial_number, &success))
			answ = 1;  // to not mistake with the errors from success
	}
	if (answ) {
		buf = "Server error during start camera recording. Code " + to_string(answ);
		logError(buf.c_str());
	}
	if (success) {
		buf = "Error during start camera recording. Code " + to_string(success);
		logError(buf.c_str());
		answ = success;
	}
	return answ;
}

void Protocol::break_camera_recording()
{
	if (m_cameraClient1.isConnected())
		m_cameraClient1.breakRecording();
	if (m_cameraClient2.isConnected())
		m_cameraClient2.breakRecording();
}

bool Protocol::did_cameras_finish_saving()
{
	bool answ = true;
	int res;
	if (m_cameraClient1.isConnected()) {
		m_cameraClient1.areYouDoneSaving(&res);
		if (res == 0)
			answ = false;
	}
	if (answ && m_cameraClient2.isConnected()) {
		m_cameraClient2.areYouDoneSaving(&res);
		if (res == 0)
			answ = false;
	}
	return answ;
}

int Protocol::wait_for_cameras_finish_saving()
{
	auto waitStart = Times::getCurrentTime();
	double timeout = 5*60; // seconds
	
	int flag = 0;
	while (true) {
		if (did_cameras_finish_saving()) {
			break;
		}
		if (Times::isTimeout(waitStart, timeout)) {
			flag = -1;
			break;
		}
	}
	return flag;
}

void Protocol::connect_pressure_sensors()
{
	m_touchSensorClient.server_ip = params.tss_ip;
	m_touchSensorClient.port = params.tss_port;

	m_touchSensorClient.connect_f();
}

void Protocol::disconnect_pressure_sensors()
{
	m_touchSensorClient.disconnect_f();
}

void Protocol::start_pressure_sensor_recording()
{
	start_pressure_sensor_recording(currentTrialNumber);
}

void Protocol::start_pressure_sensor_recording(long trial_number)
{
	if (m_touchSensorClient.isConnected()) {
		m_touchSensorClient.startRecording(trial_number);
	}
}

/// <summary>
/// Stops the recording of the pressure sensor
/// </summary>
/// <returns>Whether trial was successful. Not used in the current protocol - see monitor.</returns>
int Protocol::break_pressure_sensor_recording()
{
	if (m_touchSensorClient.isConnected()) {
		atomic<int> result;
		m_touchSensorClient.breakRecording(&result);
		return result.load();
	}
	return -1;
}

void Protocol::run()
{
	this->stopProtocol.store(false);
	this->protocolState.store(ProtocolState::initializing);  // add a GUI text field

	// TODO: Load all trials from session config file (BL code)

	// Initialize all devices
	initDevices();

	// on first start, looping should be disabled until the start trial button is pressed
	loopAutomatically = true;

	// class member variable
	currentTrialNumber = 0;
	int rets = 0;
	auto intertrialWaitStartTime = Times::getCurrentTime();  // set at the end of the previous iteration
	auto trialStartTime = Times::getCurrentTime();  // set when the object is in position

	// Run the protocol loop
	while (!this->stopProtocol.load())
	{
		// ---------------- Preparing trial
		updateCurrentTrialOnTheGui();

		// TODO: load and set the parameters of the next trial

		// TODO: if went through all trials, stop the protocol.

		// TRIAL is ready to start
		this->protocolState.store(ProtocolState::trialReady);
		this->startTrial.store(false);

		trialFieldsEnableStart(true);

		// waiting for the start of the next trial
		while (!this->startTrial.load() && 
			!this->stopProtocol.load() && 
			(loopAutomatically.load() && !Times::isTimeout(intertrialWaitStartTime, params.intertrialWaitTime))) {
			// waiting for:
			//	Start trial button to be pressed
			//  stop of protocol 
			//  if looping is selected, timeout of intertrial time. TODO: enable looping behavior toggle from GUI
			//  TODO: wait for the monkey to release the grasp on the object
		}

		// Default behavior is looping
		loopAutomatically = true;

		// Can't click on StartTrial anymore
		trialFieldsEnableStart(false);

		if (stopProtocol)
			break;

		// ---------------- Running trial
		// TRIAL in progress set state and state control variables
		this->protocolState.store(ProtocolState::trialInProgress);
		this->stopTrial.store(false);

		// prepare recordings
		send_config_to_cameras();

		// start recordings
		rets = start_camera_recording();  // TODO process it?
		start_pressure_sensor_recording();
		start_ephys_recording();

		// Can click on stop trial
		trialFieldsEnableRetreat(true);

		// TODO: while the motors go forward, check if the monkey grabbed, then immediately BOOP and return
		// TODO: change start motors functions
		if (!startForwardMovement())
		{
			// TODO: check error with motors
			break;
		}

		// spawn the process that monitors the async stopping conditions
		m_asyncTrialSuccessMonitorThread = new thread(&Protocol::m_asyncTrialConditionMonitor, this);

		trialStartTime = Times::getCurrentTime();
		Sounds::playStartTaskTone();

		// wait for stop trial signal from interface, success from the monitor thread, or timeout
		while (!this->stopProtocol.load() && 
			!this->stopTrial.load() && 
			!this->m_earnedReward.load() &&
			!Times::isTimeout(trialStartTime, params.maxWaitTime)) {}

		// if stop trial button was pressed, turn off the loop - it is reenable automatically in the beginning of trial
		if (this->stopTrial.load())
			loopAutomatically = false;

		// Cannot interrupt the trial
		trialFieldsEnableRetreat(false);

		// ---------------- Finishing trial
		this->protocolState.store(ProtocolState::trialFinalizing);

		// stop monitor thread if it was not stopped by its own success
		m_stopAsyncTrialConditionMonitor = true;

		// Give the reward or not
		if (m_earnedReward || deservesReward) {
			reward();
		}
		else {
			Sounds::playErrorTone();
		}

		// retreat motors
		if (params.tstEnMotors) {
			// retreat back
			motorHub->retreat();
		}

		// stop recording
		break_camera_recording();
		break_pressure_sensor_recording();
		break_ephys_recording();

		// countdown for next trial
		intertrialWaitStartTime = Times::getCurrentTime();

		// wait for the signal from recording devices that the data has been saved - is Ready
		rets = wait_for_cameras_finish_saving();
		if (rets < 0) {
			AfxMessageBox("Cameras are taking too long to save the data. Stopping the protocol.");
			break;
		}


		// TODO display the state of the trial on the GUI

		currentTrialNumber++;
	}
	this->protocolState.store(ProtocolState::shuttingDown);

	trialFieldsEnableStart(false);
	trialFieldsEnableRetreat(false);

	// retreat motors
	if (params.tstEnMotors) {
		// TODO: retreat back
		motorHub->retreat();
	}

	// release all devices
	releaseDevices();

	this->protocolState.store(ProtocolState::shutdown);
}

void Protocol::set_photoresistor_monitors(CStaticColor* front, CStaticColor* rear)
{
	// does not care if the card is there or whatever
	m_NIUsb6001card.setFrontPhotoresistorMonitor(front);
	m_NIUsb6001card.setRearPhotoresistorMonitor(rear);
}

void Protocol::set_camera1_gui_controls(CEdit* serverStatusCtrl, CEdit* serverLogCtrl)
{
	m_cameraClient1.clientStatusGuiEdt = serverStatusCtrl;
	m_cameraClient1.clientLogGuiEdt = serverLogCtrl;
}

void Protocol::set_camera2_gui_controls(CEdit* serverStatusCtrl, CEdit* serverLogCtrl)
{
	m_cameraClient2.clientStatusGuiEdt = serverStatusCtrl;
	m_cameraClient2.clientLogGuiEdt = serverLogCtrl;
}

void Protocol::set_pressure_sensors_gui_controls(CEdit* serverLogCtrl)
{
	m_touchSensorClient.clientLogGuiEdt = serverLogCtrl;
}

void Protocol::set_current_trial_gui_control(CEdit* currentTrialGuiCtrl)
{
	m_currentTrialGuiCtrl = currentTrialGuiCtrl;
}

void Protocol::set_trial_buttons(CButton* startTrialBtn, CButton* retreatBtn, CButton* retreatFlushBtn)
{
	this->startTrialBtn = startTrialBtn;
	this->retreatBtn = retreatBtn;
	this->retreatFlushBtn = retreatFlushBtn;
}

void Protocol::trialFieldsToggle(bool enable)
{
	trialFieldsEnableStart(enable);
	trialFieldsEnableRetreat(!enable);
}

void Protocol::trialFieldsEnableStart(bool enable)
{
	startTrialBtn->EnableWindow(enable);
}

void Protocol::trialFieldsEnableRetreat(bool enable)
{
	retreatBtn->EnableWindow(enable);
	retreatFlushBtn->EnableWindow(enable && params.tstEnReward);
}

void Protocol::logGoodTrial(const long& nCurrentTrial, const long& microsecsFromStartTaskToneToLiftingMonkeyArm, const long& microsecsFromMonkeyArmRaisedToPlatesTouching)
{
	string msg = TRIAL_NUM_STR + to_string(nCurrentTrial);
	msg = msg + " monkey raised its arm in [microsecs]: " + to_string(microsecsFromStartTaskToneToLiftingMonkeyArm) + " -> monkey touched the plates in [microsecs]: " + to_string(microsecsFromMonkeyArmRaisedToPlatesTouching);
	logInfo(msg.c_str());
}

void Protocol::logBadTrial(const long& nCurrentTrial)
{
	string msg = TRIAL_NUM_STR + to_string(nCurrentTrial);
	msg = msg + TRIAL_ABORT_STR;
	logError(msg.c_str());
}

void Protocol::initDevices()
{
	// NI card: photoresistors, motor and reward
	if (params.isNiCardBeingUsed()) {
		m_NIUsb6001card.config();

		m_NIUsb6001card.start();
	}

	// motor
	if (params.tstEnMotors) {
		if (motorHub) {
			// TODO WARNING: motor hub already initialized
		}
		else {
			motorHub = new MotorAPI();

			// TODO: home the motors once
			motorHub->home();
		}
	}
}

void Protocol::releaseDevices()
{
	// NI card
	if (params.isNiCardBeingUsed())
		m_NIUsb6001card.stop();

	// photoresistors
	if (params.tstEnLightSensors)
		m_NIUsb6001card.resetPhotoresistorsGuiMonitor();

	// motor
	if (params.tstEnMotors) {
		if (motorHub) {  // check if nullptr
			delete motorHub;
			motorHub = nullptr;
		}
	}
}

// TODO rename and change. Is it used?
bool Protocol::isMotorMovementAborted()
{
	if (params.tstEnMotors) {
		// TODO: retreat back
		motorHub->retreat();
	}

	// on waiting for the monkey puts the arm on the armrest before to start the trial
	//while (!stopProtocol->load() && ( !IS_REAR_PHOTORESISTOR_COVERED || !IS_FRONT_PHOTORESISTOR_COVERED)) {}

	if (stopProtocol.load())
		return true;
	// return true -> go() aborted
	if (params.tstEnMotors) {
		// TODO change
		vector<int> pos = { params.position , 1000, 1000 };
		motorHub->move(pos);
	}
	
	return true;
}

/// <summary>
///
/// </summary>
/// <returns>True iff the motor movement started as planned or no motor initialized via testing</returns>
bool Protocol::startForwardMovement()
{
	if (params.tstEnMotors) {
		// make an API RETREAT movement.
		motorHub->retreat();
	}

	if (this->stopProtocol.load() || this->stopTrial.load())
		return false;

	if (params.tstEnMotors) {
		// TODO
		vector<int> pos = { params.position , 1000, 1000 };
		motorHub->move(pos);
	}
	return true;
}

void Protocol::start_ephys_recording()
{
	// wrap in case the ephys is not connected
	if (params.tstEnEphys)
		m_NIUsb6001card.ephysSyncStart();  
}

void Protocol::break_ephys_recording()
{
	// wrap in case the ephys is not connected
	if (params.tstEnEphys)
		m_NIUsb6001card.ephysSyncStop();
}

void Protocol::updateCurrentTrialOnTheGui()
{
	if (m_currentTrialGuiCtrl) {
		CStringA nTrialsConverted;
		nTrialsConverted.Format(_T(PRECISION), currentTrialNumber.load());
		m_currentTrialGuiCtrl->SetWindowText(nTrialsConverted);
	}
}

/// <summary>
/// Asks Pressure Sensor if the reward has been earned, ends when sets m_earnedReward to true
/// TODO: Pressure sensor constantly returns whether the grab is occuring along with the success of the trial,
/// and if so happens before the grasp do not give reward
/// </summary>
void Protocol::m_asyncTrialConditionMonitor()
{
	m_earnedReward = false;
	m_stopAsyncTrialConditionMonitor = false;

	std::atomic<int> result;
	atomic<double> leftForce;
	atomic<double> rightForce;
	while (!m_stopAsyncTrialConditionMonitor) {
		if (m_touchSensorClient.isConnected()) {
			// ask touch sensor if the plates are being grabbed
			// TODO question
			// deprecated: m_touchSensorClient.checkSuccess(&result);

			// TODO implement the logic
			// minimum force level of 0.2 of desired and total excedes the desired
			if (result > 0) {
				m_earnedReward = true;
				m_stopAsyncTrialConditionMonitor = true;
			}
		}

	}
}
