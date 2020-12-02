#include "Protocol.h"

constexpr auto DATA_FOLDER = "./data";
constexpr auto TRIAL_NUM_STR = "Trial n.";
constexpr auto TRIAL_ABORT_STR = " Aborted";
constexpr auto PRECISION = "%03d";

constexpr auto MIN_UNCOVERED_TIME = 200; //msecs


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

void Protocol::capture_single_frame()
{
	send_config_to_cameras();
	if (m_cameraClient1.isConnected())
		m_cameraClient1.captureSingleFrame();
	if (m_cameraClient2.isConnected())
		m_cameraClient2.captureSingleFrame();
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

void Protocol::start_camera_recording()
{
	start_camera_recording(currentTrialNumber.load());
}

void Protocol::start_camera_recording(long trial_number)
{
	if (m_cameraClient1.isConnected()) {
		m_cameraClient1.startRecording(trial_number);
	}
	if (m_cameraClient2.isConnected()) {
		m_cameraClient2.startRecording(trial_number);
	}
}

void Protocol::break_camera_recording()
{
	if (m_cameraClient1.isConnected())
		m_cameraClient1.breakRecording();
	if (m_cameraClient2.isConnected())
		m_cameraClient2.breakRecording();
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

int Protocol::break_pressure_sensor_recording()
{
	if (m_touchSensorClient.isConnected()) {
		atomic<int> result;
		m_touchSensorClient.breakRecording(&result);
		return result.load();
	}
	return -1;
}

Protocol::Protocol()
{
	this->stopProtocol.store(false);
	this->startTrial.store(false);
	this->stopTrial.store(false);

	this->protocolState.store(ProtocolState::shutdown);
}

Protocol::~Protocol()
{
	// if incorrect termination
	releaseDevices();
}

void Protocol::run()
{
	this->stopProtocol.store(false);
	this->protocolState.store(ProtocolState::initializing);
	// TODO?: Create directory for log storage
	CreateDirectory(DATA_FOLDER, NULL);

	// TODO: Load all trials from session config file

	// Initialize all devices
	initDevices();

	// class member variable
	currentTrialNumber = 0;
	steady_clock::time_point intertrialWaitStartTime = Times::getCurrentTime();  // set at the end of the previous iteration
	steady_clock::time_point trialStartTime;  // set when the object is in position

	// Run the protocol loop
	while (!this->stopProtocol.load())
	{
		// ---------------- Preparing trial
		updateCurrentTrialOnTheGui();

		// TODO: load the parameters of the next trial

		// set state and state control variables
		this->protocolState.store(ProtocolState::trialReady);
		this->startTrial.store(false);

		// waiting for the start of the next trial
		while (!this->startTrial.load() && !this->stopProtocol.load() && 
			!Times::isTimeout(intertrialWaitStartTime, params.intertrialWaitTime)) {
			// waiting for:
			//	Start trial button to be pressed
			//  stop of protocol 
			//  if looping is selected, timeout of intertrial time. TODO: enable looping behavior toggle
		}

		if (stopProtocol)
			break;

		// ---------------- Running trial
		// set state and state control variables
		this->protocolState.store(ProtocolState::trialInProgress);
		this->stopTrial.store(false);

		// prepare recordings
		send_config_to_cameras();
		prepare_camera_recording();  // does not do anything atm

		// start recordings
		start_camera_recording();
		start_pressure_sensor_recording();
		start_ephys_recording();

		// spawn the process that monitors the async stopping conditions
		m_asyncTrialSuccessMonitorThread = new thread(&Protocol::m_asyncTrialConditionMonitor, this);

		// TODO: change start motors functions
		if (!startForwardMovement())
		{
			// TODO: error with motors
			break;
		}

		Sounds::playStartTaskTone();
		trialStartTime = Times::getCurrentTime();

		// wait for stop trial signal from interface or monitor thread, or timeout
		while (!this->stopProtocol.load() && !this->stopTrial.load() && 
			!Times::isTimeout(trialStartTime, params.maxWaitTime)) {}

		// ---------------- Finishing trial
		this->protocolState.store(ProtocolState::trialFinalizing);

		// stop monitor thread if it was running
		m_stopAsyncTrialConditionMonitor = true;

		// TODO: calculate if the grab happened too early - see comment in the monitor function
		if (m_earnedReward) {
			
		}

		// Give the reward or not
		if (m_earnedReward || deservesReward) {
			reward();
		}
		else {
			Sounds::playErrorTone();
		}

		// retreat motors TODO: check if correct way
		if (params.tstEnMotors) {
			motorHub->reset();
			motorHub->home();
		}

		// stop recording
		break_camera_recording();
		break_pressure_sensor_recording();
		break_ephys_recording();

		// countdown for next trial
		intertrialWaitStartTime = Times::getCurrentTime();

		// TODO wait for the signal from recording devices that the data has been saved - is Ready

		// TODO display the progress of saving on the GUI

		currentTrialNumber++;
	}
	this->protocolState.store(ProtocolState::shuttingDown);

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
			motorHub = new TeknicMotorDevice();
			motorHub->init();
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

bool Protocol::isMotorMovementAborted(atomic<bool> * stopProtocol)
{
	if (params.tstEnMotors)
		motorHub->reset();
	if (!stopProtocol->load() && params.tstEnMotors)
		motorHub->home();
	// on waiting for the monkey puts the arm on the armrest before to start the trial
	//while (!stopProtocol->load() && ( !IS_REAR_PHOTORESISTOR_COVERED || !IS_FRONT_PHOTORESISTOR_COVERED)) {}

	if (stopProtocol->load())
		return true;
	// return true -> go() aborted
	if (params.tstEnMotors)
		return motorHub->go(&params.position, &params.speed, &params.acceleration);
	else
		return true;
}

/// <summary>
///
/// </summary>
/// <param name="stopProtocol"></param>
/// <param name="stopTrial"></param>
/// <param name="m_NIUsb6001card"></param>
/// <param name="motorHub"></param>
/// <returns>True iff the motor movement started as planned or no motor initialized via testing</returns>
bool Protocol::startForwardMovement()
{
	if (params.tstEnMotors) {
		motorHub->reset();
		motorHub->home();
	}

	if (this->stopProtocol.load() || this->stopTrial.load())
		return false;

	if (params.tstEnMotors)
		return !motorHub->go(&params.position, &params.speed, &params.acceleration);
	else
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
	while (!m_stopAsyncTrialConditionMonitor) {
		// ask touch sensor if success
		if (m_touchSensorClient.isConnected()) {
			m_touchSensorClient.checkSuccess(&result);

			if (result > 0) {
				m_earnedReward = true;
				m_stopAsyncTrialConditionMonitor = true;
			}
		}

	}
}
