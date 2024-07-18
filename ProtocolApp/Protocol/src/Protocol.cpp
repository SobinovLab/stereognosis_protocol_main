#include "Protocol.h"

using namespace std;

constexpr auto PRECISION = "%03d";

// TODO CR, Implement server (i) methods for start/stop/sync etc

Protocol::Protocol()
{
	this->stopProtocol.store(false);
	this->startTrial.store(false);
	this->stopTrial.store(false);

	this->deservesReward.store(false);
	this->loopAutomatically.store(true);

	this->protocolState.store(ProtocolState::shutdown);

	// test if things can be enabled and then change the variables
	initDevices();

	CameraClient* cc = new CameraClient();

	// HACK initialize camera clients
	for (size_t i = 0; i < NUM_CAMS; i++)
	{
		m_cameraClients.push_back(new CameraClient);
	}
	logInfo("Finished with INIT");
}

Protocol::~Protocol()
{
	releaseDevices();
	closeCsvLog();
}

void Protocol::setCurrentState(ProtocolState state)
{
	protocolState = state;
	trialStateGuiUpdate();
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
	m_NIUsb6001card.reward(duration);
}

bool Protocol::were_motors_homed()
{
	//return motorHub->wereHomed();
    return true; //with the Arm, homing is not reqcuired
}

int Protocol::home_motors()
{
	//return motorHub->home();
    return armClient->goToHome();
}

void Protocol::stop_motors()
{
    armClient->stopArm();
	//motorHub->stop();
}

int Protocol::motors_neutral_position()
{
	//return motorHub->neutral_position();
    return armClient->goToHome();
}


void Protocol::connect_camera_i_client(int i) {
	if (m_cameraClients[i]->isConnected()) {
		// warning?
	}
	else {
		// Assign the ip and port
		m_cameraClients[i]->server_ip = params.cs_ips[i];
		m_cameraClients[i]->port = params.cs_port;
		m_cameraClients[i]->connect_f();
	}
}


void Protocol::disconnect_camera_i_client(int i) {
	m_cameraClients[i]->disconnect_f();
}


void Protocol::start_calibration_recording()
{
	// Send hardcoded params
	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (m_cameraClients[i]->isConnected()) {
			m_cameraClients[i]->sendFramerate(10);
			m_cameraClients[i]->sendRecordingPeriod(600);
			m_cameraClients[i]->sendReferenceCamera(params.cs_refSerial);
			m_cameraClients[i]->sendGain(params.cs_gain);
			m_cameraClients[i]->sendExposure(params.cs_exposure);
		}
	}

	prepare_camera_recording();
	start_camera_recording(999); // trial num 999
	logInfo("Started calibration recording");
}

void Protocol::stop_calibration_recording() {
	break_camera_recording();
	//bool finished = did_cameras_finish_saving();
}


void Protocol::send_config_to_cameras()
{
	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (m_cameraClients[i]->isConnected()) {
			m_cameraClients[i]->sendFramerate(params.cs_framerate);
			m_cameraClients[i]->sendRecordingPeriod(params.cs_recordingPeriod);
			m_cameraClients[i]->sendReferenceCamera(params.cs_refSerial);
			m_cameraClients[i]->sendGain(params.cs_gain);
			m_cameraClients[i]->sendExposure(params.cs_exposure);
		}
	}

}


int Protocol::capture_single_frame()
{
	send_config_to_cameras();

	int success = 0;
	int answ = 0;
	string buf;

	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (!m_cameraClients[i]->captureSingleFrame(&success))
		{
			answ = 1;  // to not mistake with the errors from success
		}
	}

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
	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (m_cameraClients[i]->isConnected())
		{
			m_cameraClients[i]->prepareRecording();
		}
	}
}


int Protocol::start_camera_recording()
{
	return start_camera_recording(params.counter);
}


int Protocol::start_camera_recording(long trial_number)
{
	// NB config is sent separately in the main loop
	int success = 0;
	int answ = 0;  // cameras not connected is not an error
	string buf;

	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (m_cameraClients[i]->isConnected())
		{
			if (!m_cameraClients[i]->startRecording(trial_number, &success))
				answ = 1;  // to not mistake with the errors from success
		}
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
	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (m_cameraClients[i]->isConnected())
		{
			m_cameraClients[i]->breakRecording();
		}
	}
}


bool Protocol::did_cameras_finish_saving()
{
	int res;

	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (m_cameraClients[i]->isConnected())
		{
			m_cameraClients[i]->areYouDoneSaving(&res);
			if (res == 0)
				return false;
		}
	}

	return true;
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

void Protocol::sync_message_trial_start()
{

	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (m_cameraClients[i]->isConnected())
		{
			m_cameraClients[i]->syncMessageTrialStart();
		}
	}

	if (m_touchSensorClient.isConnected()) {
		m_touchSensorClient.syncMessageTrialStart();
	}
}

void Protocol::sync_message_trial_end()
{
	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (m_cameraClients[i]->isConnected())
		{
			m_cameraClients[i]->syncMessageTrialEnd();
		}
	}

	if (m_touchSensorClient.isConnected()) {
		m_touchSensorClient.syncMessageTrialEnd();
	}
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

void Protocol::push_variables_to_gui()
{
	if (mainWindow)
		mainWindow->UpdateData(FALSE);
}

void Protocol::pull_variables_from_gui()
{
	if (mainWindow)
		mainWindow->UpdateData(TRUE);
}

void Protocol::start_pressure_sensor_recording()
{
	start_pressure_sensor_recording(params.counter);
}

void Protocol::start_pressure_sensor_recording(long counter)
{
	if (m_touchSensorClient.isConnected()) {
		m_touchSensorClient.startRecording(counter);
	}
}

// CR: added to send the timestamp so that we can update the output directory name in the pressure server
void Protocol::send_pressure_sensor_timestamp(int timestamp)
{
	// Follow the logic of above to check that we have connected to the ps server
	if (m_touchSensorClient.isConnected()) {
		m_touchSensorClient.setTimestamp(timestamp);
	}
}

void Protocol::send_camera_timestamp(int timestamp) {
	for (int i = 0; i < m_cameraClients.size(); i++) {
		if (m_cameraClients[i]->isConnected())
		{
			m_cameraClients[i]->setTimestamp(timestamp);
		}
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

void Protocol::wait_until_monkey_release()
{
	atomic<double> leftForce = 0;
	atomic<double> rightForce = 0;
	while (m_touchSensorClient.isConnected()) {
		// ask pressure sensor for pressure
		m_touchSensorClient.getForce(&leftForce, &rightForce);

		if (leftForce + rightForce < params.minimalTouchForce)
			break;

		// can start manually
		if (startTrial)
			break;
	}
}

void Protocol::playStartTaskTone()
{
	if (params.sounds_modulation_enabled) {
		double normedForce = (params.targetForce - params.sounds_minforce) / (params.sounds_maxforce - params.sounds_minforce);
		normedForce = max(min(1.0, normedForce), 0.0);
		Sounds::playTone(normedForce * (params.sounds_maxfreq - params.sounds_minfreq) + params.sounds_minfreq);
	}
	else
		Sounds::playStartTaskTone();
}

// This is what is run on Start Protocol Button
void Protocol::run()
{
	logInfo("Starting protocol");
	this->stopProtocol.store(false);
	setCurrentState(ProtocolState::initializing); // display the state of the trial on the GUI
	int rets = 0;
	TEKNIC_MOTOR_API_CODE motor_rets = TEKNIC_MOTOR_API_CODE::OK;
    int motorRet;

	// CR: this is the timestamp we want to send from the main protocol to the pressure and cameras server
	// Convert the time point to a time_t object
	// Get the current time point
	auto currentTimePoint = std::chrono::system_clock::now();
	std::time_t currentTime = std::chrono::system_clock::to_time_t(currentTimePoint);
	int currentTimeInSeconds = static_cast<int>(currentTime);
	// Send ps and cam server timestamp
	send_pressure_sensor_timestamp(currentTimeInSeconds);
	send_camera_timestamp(currentTimeInSeconds);

	// Load all trials from session config file (BL code)
	vector<string> session_line1;
	vector<string> session_line2;
	vector<vector<double>> session_values;
	vector<bool> repeating_trial;
	bool usingLoadedSession;
	rets = CsvParser::parseCSV(string(params.session_filename), session_line1, session_line2, session_values);
	if (rets) {  // could not load
		usingLoadedSession = false;
		params.total_trials = 0;
		forceTargetIds.clear();
		forceTargets.clear();
	}
	else {
		usingLoadedSession = true;
		params.total_trials = session_values.size();
		for (size_t i = 0; i < params.total_trials; i++)
			repeating_trial.push_back(false);
		forceTargetIds = CsvParser::getForceTargets(session_line1);
		forceTargets.clear();
	}

	// on first start, looping should be disabled until the start trial button is pressed
	autoLoopToggle(true);
	loopAutomatically = false;
	bool arm_at_rest;

	// open csv file for logging
	openCsvLog();
	long long trial_start_time;
    long long log_sent_config_to_cameras;
	long long object_in_position_time;
	long long arm_liftoff_time;
    long long log_started_camera_recording;
    long long log_started_ps_recording;
    long long log_started_ephys_recording;
	long long log_sent_start_sync_messages;
    long long log_started_monitoring_ps;
	long long trial_end_time;
    long long arm_return_time;
    long long log_starting_finishing_recordings;
	long long log_sent_end_sync_messages;
    long long log_stopped_camera_recordings;
    long long log_stopped_ps_recordings;
	long long trial_finished_time;

	// class member variable
	params.counter = 0;
	params.trial_number = 0;

	int preset_trial_number;
	auto intertrialWaitStartTime = Times::getCurrentTime();  // set at the end of the previous iteration
	auto trialStartTime = Times::getCurrentTime();  // set when the object is in position

	// Run the protocol loop
	logInfo("About to go into the main loop");
	while (!this->stopProtocol.load())
	{
		// ---------------- Preparing trial
		trial_start_time = 0;
        log_sent_config_to_cameras = 0;
		object_in_position_time = 0;
		arm_liftoff_time = -1;
        log_started_camera_recording = 0;
        log_started_ps_recording = 0;
        log_started_ephys_recording = 0;
		log_sent_start_sync_messages = 0;
        log_started_monitoring_ps = 0;
		m_force_target_start_times.clear();
		m_started_touching_times.clear();
		m_started_touching_times.push_back(0);
		trial_end_time = 0;
        log_starting_finishing_recordings = 0;
		log_sent_end_sync_messages = 0;
        log_stopped_camera_recordings = 0;
        log_stopped_ps_recordings = 0;
		trial_finished_time = 0;

		// if went through all trials, break the loop
		if (params.total_trials > 0 && params.trial_number >= params.total_trials)
			break;

		// load and set the parameters of the next trial
		preset_trial_number = params.trial_number;
		if (usingLoadedSession) {
			matchLoadedSessionTrialToParams(session_line1, session_line2, session_values[params.trial_number]);
			params.total_trials = session_values.size();  // in case a failed trial was pushed to the end
		}

		// update the gui with set values and current trial #
		push_variables_to_gui();

		// TRIAL is ready to start
		setCurrentState(ProtocolState::trialReady);
		this->startTrial.store(false);

		trialFieldsEnableStart(true);

		// wait for the monkey to release the grasp on the object - can be forced by startTrial
		wait_until_monkey_release();

		//// wait until arm at rest
		//if (wait_until_arm_at_rest() < 0) {
		//	// timeout for 20 mins
		//	stopTrial = true;
		//	stopProtocol = true;
		//}
		// instead, now this:
		logInfo("Resting bullshit");
		prepare_to_monitor_arm_at_rest();
		arm_at_rest = false;

		// waiting for the start of the next trial
		logInfo("Waiting for trial to start");
		char msg[256];
		while (!this->startTrial.load() && !this->stopProtocol.load()) {
			// waiting for:
			//	Start trial button to be pressed
			//  stop of protocol

			// once the state has switched to covered - arm is at rest
			if (!arm_at_rest)
				arm_at_rest = is_arm_at_rest();

			// ony wait for
			//  if looping is selected, timeout of intertrial time
			//sprintf(msg, "Loop automatically: %i, Times: %i, arm_at_rest: %i", loopAutomatically.load(), Times::isTimeout(intertrialWaitStartTime, params.intertrialWaitTime), arm_at_rest);
			//logInfo(msg);
			if (loopAutomatically.load() && Times::isTimeout(intertrialWaitStartTime, params.intertrialWaitTime) && arm_at_rest)
				break;
		}
		logInfo("Got through that wait");
		// The usual place to exit the protocol, if not at the end of a trial
		if (stopProtocol.load())
			break;
		// Default behavior is looping - after the first trial
		autoLoopToggle(true);

		// GUI Can't click on StartTrial anymore
		trialFieldsEnableStart(false);

		// just in case any parameters changed, pull from GUI
		// might update the first target
		pull_variables_from_gui();

		// if the trial changed, load it instead
		if (usingLoadedSession && preset_trial_number != params.trial_number && params.trial_number < params.total_trials) {
			matchLoadedSessionTrialToParams(session_line1, session_line2, session_values[params.trial_number]);
			// update the gui if trial was changed
			push_variables_to_gui();
		}
		// ---------------- Running trial
		string buf = "Upcoming position " + to_string(params.pos_translation_x) + " " +
			to_string(params.pos_tilt) + " " + to_string(params.pos_aperture);
		logInfo(buf.c_str());

		// TRIAL in progress set state and state control variables
		setCurrentState(ProtocolState::trialInProgress);
		this->stopTrial.store(false);
		// has not earned anything yet
		m_earnedReward = false;
		deservesReward = false;

		// log
		trial_start_time = Times::getCurrentTimeInMilliSecs();

		// sync
		start_ephys_recording();
		log_started_ephys_recording = Times::getCurrentTimeInMilliSecs();
		sync_message_trial_start();
		log_sent_start_sync_messages = Times::getCurrentTimeInMilliSecs();

		// prepare recordings
		send_config_to_cameras();
        log_sent_config_to_cameras = Times::getCurrentTimeInMilliSecs();

		// GUI Can click on stop trial
		trialFieldsEnableRetreat(true);

		// motor movement - this thread will be locked, can be interrupted
		// The motors API only supports position control as dynamics are not important
		// TODO make gui list-based
		//vector<string> axes = { "translation_X", "tilt", "aperture" };
		//vector<double> positions = { params.pos_translation_x, params.pos_tilt, params.pos_aperture };
		rets = 0;
		motorRet = 1;
		if (!stopTrial.load())
			motorRet = armClient->preshape(params.pos_aperture);
		if (motorRet < 0)
		{
			// bad error
			logError("Bad motor error encountered during preshape. Interrupting the protocol. Error is the following:");
            logError(checkKinovaErrCode(motorRet).c_str());
			rets = -1;
		}

		// show target force
		if (params.leds_early_target_force_lightup && isLedsOn() && m_touchSensorClient.isConnected()) {
			// unchanging definitions - same as in the Monitor
			double targetForceMin = std::max(params.targetForce + params.targetForceRelRangeMin, params.targetForceTotalMinThreshold);
			double targetForceMax = params.targetForce + params.targetForceRelRangeMax;

			// target force range
			ledStrip->set_top_stripe_lights(
				targetForceMin / params.targetForceTotalMax,
				targetForceMax / params.targetForceTotalMax);
		}

		// wait until arm at rest
		prepare_to_monitor_arm_at_rest();

		logInfo("About to wait for arms to be at rest at top of loop");
		while (!this->stopTrial.load() && !is_arm_at_rest()) {
			// waiting for:
			//	Stop trial button to be pressed
			//  stop of protocol
			//  arm at rest
		}

		// start a thread that asks if monkey grabbed/lifted arm and stopTrial=1 if it did
		stopWatch = false;
		thread watchThread(&Protocol::watch_early_grab, this);

        allowInterupt.store(true);
		logInfo("Starting background thread for passive arm");
        thread passiveArmThread(&Protocol::armMonitoringThread, this);

		// start recordings
		// Calc trial sub number
		start_camera_recording(params.counter);  // TODO process it?
		log_started_camera_recording = Times::getCurrentTimeInMilliSecs();
		start_pressure_sensor_recording(params.counter);
		log_started_ps_recording = Times::getCurrentTimeInMilliSecs();

		// approach
		if (!rets && !stopTrial.load()) {
			//motor_rets = motorHub->approach(axes, positions);
            motorRet = armClient->moveToPosition(params.pos_translation_x, params.pos_translation_depth, params.pos_translation_height, params.pos_tilt, 0, 0, params.pos_aperture);
			if (motorRet < 0)
			{
                logError("Bad motor error encountered during approach. Interrupting the protocol. Error is the following");
                logError(checkKinovaErrCode(motorRet).c_str());
                rets = -1;
			}
		}

		// log
		if (!stopTrial.load())
			object_in_position_time = Times::getCurrentTimeInMilliSecs();

		// stop the watch thread
		stopWatch = true;
		watchThread.join();

		// if the trial was not interrupted, wait for the hand liftoff
		if (!stopTrial.load() && !rets) {
			// TODO in the current structure does not make sense
			//rets = wait_until_arm_liftoff();

			//arm_liftoff_time = Times::getCurrentTimeInMilliSecs();
		}

		// if the motors made it successfully to the final position and the animal has lifted the arm
		if (!stopTrial.load() && rets >= 0) {
			bool animalLifted = false;
			// spawn the process that monitors the async stopping conditions
			m_asyncTrialSuccessMonitorThread = new thread(&Protocol::m_asyncTrialConditionMonitor, this);
            log_started_monitoring_ps = Times::getCurrentTimeInMilliSecs();
			trialStartTime = Times::getCurrentTime();
			playStartTaskTone();

			// wait for stop trial signal from interface, success from the monitor thread, or timeout
			while (!this->stopTrial.load() &&
				!this->m_earnedReward.load() &&
				!Times::isTimeout(trialStartTime, params.maxWaitTime)) {

				if(!animalLifted && ( (use_left_arm_touch && (which_active_arm==-1) && !IS_LEFT_ARMSENSOR_TOUCHED ) || (use_right_arm_touch && (which_active_arm == 1) && !IS_RIGHT_ARMSENSOR_TOUCHED)))
				{
					animalLifted = true;
					arm_liftoff_time = Times::getCurrentTimeInMilliSecs();
				}
			}
			// if stop trial button was pressed, turn off the loop - it is reenable automatically in the beginning of trial
			if (this->stopTrial.load() && params.disable_looping_on_manual_retreat)
				autoLoopToggle(false);  // want to stop only for one trial

			// stop monitor thread if it was not stopped by its own success
			m_stopAsyncTrialConditionMonitor = true;
			// not necessary, but cleaner:
			if (m_asyncTrialSuccessMonitorThread) {
				m_asyncTrialSuccessMonitorThread->join();
				delete m_asyncTrialSuccessMonitorThread;
				m_asyncTrialSuccessMonitorThread = nullptr;
			}
		}

		logInfo("Setting interupt from main loop");
        allowInterupt.store(false);
		logInfo("Waiting on background thread");
        passiveArmThread.join();

		// turn off leds whether they were on or not
		if (isLedsOn()) {
			ledStrip->turn_off_both_stripe_lights();
		}

		// GUI Cannot interrupt the rest of trial
		trialFieldsEnableRetreat(false);

		// ---------------- Finishing trial
		setCurrentState(ProtocolState::trialFinalizing);

		// log
		trial_end_time = Times::getCurrentTimeInMilliSecs();

		// Give the reward or not
		if (m_earnedReward || deservesReward) {
			reward();
            if(reward_on_return.load())
            {
                prepare_to_monitor_arm_at_rest();
				logInfo("Waiting for arms to be placed back so that second reward can happen");
		        while (!this->stopTrial.load() && !is_arm_at_rest()) {
			        //TODO maybe add timeout here
                    //This waits for monkey to put both arms back and then rewards again
		        }
                arm_return_time = Times::getCurrentTimeInMilliSecs();
                reward();
            }
		}
		else {
			Sounds::playErrorTone();

			// append the failed trial to the end
            arm_return_time = -1;
			if (usingLoadedSession) {
				session_values.push_back(session_values[params.trial_number]);
				repeating_trial.push_back(true);
			}
		}

		// retreat motors
		//motorHub->retreat();
        armClient->goToHome();
		log_starting_finishing_recordings = Times::getCurrentTimeInMilliSecs();

		// sync again
		break_ephys_recording();
		sync_message_trial_end();
		log_sent_end_sync_messages = Times::getCurrentTimeInMilliSecs();

		// stop recording
		break_camera_recording();
        log_stopped_camera_recordings = Times::getCurrentTimeInMilliSecs();
		break_pressure_sensor_recording();
        log_stopped_ps_recordings = Times::getCurrentTimeInMilliSecs();

		// countdown for next trial
		intertrialWaitStartTime = Times::getCurrentTime();

		// log the trial success, target positions and times
		trial_finished_time = Times::getCurrentTimeInMilliSecs();

		addLineToCsvLog(
			m_earnedReward || deservesReward,
			repeating_trial[params.trial_number],
			trial_start_time, log_sent_config_to_cameras, object_in_position_time,
            arm_liftoff_time,
            log_started_camera_recording, log_started_ps_recording,
            log_started_ephys_recording, log_sent_start_sync_messages,
			log_started_monitoring_ps,
            trial_end_time,
            log_starting_finishing_recordings, log_sent_end_sync_messages,
			log_stopped_camera_recordings, log_stopped_ps_recordings,
            trial_finished_time, arm_return_time);


		// wait for the signal from recording devices that the data has been saved - is Ready
		rets = wait_for_cameras_finish_saving();
		if (rets < 0) {
			AfxMessageBox("Cameras are taking too long to save the data. Stopping the protocol.");
			stopTrial.store(true);
			stopProtocol = true;
		}

		params.trial_number++;           // Increment trial number
		params.counter++;
	}
	setCurrentState(ProtocolState::shuttingDown);

	trialFieldsEnableStart(false);
	trialFieldsEnableRetreat(false);

	// retreat motors
	//motorHub->retreat();
    armClient->goToHome();

	// release all devices in the destructor
	closeCsvLog();

	setCurrentState(ProtocolState::shutdown);
}

void Protocol::set_photoresistor_monitors(CStaticColor* front, CStaticColor* rear, CStaticColor* left, CStaticColor* right)
{
	// does not care if the card is there or whatever
	m_NIUsb6001card.setFrontPhotoresistorMonitor(front);
	m_NIUsb6001card.setRearPhotoresistorMonitor(rear);
    m_NIUsb6001card.setArmTouchSensors(left, right);
}


// TODO route output elsewhere
void Protocol::set_camera1_gui_controls(CEdit* serverLogCtrl)
{
	//m_cameraClient1.clientLogGuiEdt = serverLogCtrl;
}

void Protocol::set_camera2_gui_controls(CEdit* serverLogCtrl)
{
	//m_cameraClient2.clientLogGuiEdt = serverLogCtrl;
}


void Protocol::set_pressure_sensors_gui_controls(CEdit* serverLogCtrl)
{
	m_touchSensorClient.clientLogGuiEdt = serverLogCtrl;
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
	retreatFlushBtn->EnableWindow(enable && isRewardOn());
}

void Protocol::matchLoadedSessionTrialToParams(const vector<string>& line1, const vector<string>& line2, const vector<double>& vec)
{
	string axis;
	string deriv;
	for (size_t i_col = 0; i_col < line1.size(); i_col++)
	{
		axis = line1[i_col];
		deriv = line2[i_col];

		// TODO make list-based GUI
		if (axis == "translation_X") {
			if (deriv == "position") {
				params.pos_translation_x = vec[i_col];
			}
		}

		if (axis == "tilt") {
			if (deriv == "position") {
				params.pos_tilt = vec[i_col];
			}
		}

		if (axis == "aperture") {
			if (deriv == "position") {
				params.pos_aperture = vec[i_col];
			}
		}

        if (axis == "translation_Y") {
			if (deriv == "position") {
				params.pos_translation_depth = vec[i_col];
			}
		}

        if (axis == "translation_Z") {
			if (deriv == "position") {
				params.pos_translation_height = vec[i_col];
			}
		}

        if (axis == "yaw") {
			if (deriv == "position") {
				params.pos_yaw = vec[i_col];
			}
		}

        if (axis == "pitch") {
			if (deriv == "position") {
				params.pos_pitch = vec[i_col];
			}
		}

		if (axis == "total_force_rel_min_bound") {
			if (deriv == "position") {
				params.targetForceRelRangeMin = vec[i_col];
			}
		}

		if (axis == "total_force_rel_max_bound") {
			if (deriv == "position") {
				params.targetForceRelRangeMax = vec[i_col];
			}
		}
	}

	// process force targets
	forceTargets.clear();
	if (!forceTargetIds.empty()) {
		params.targetForce = vec[forceTargetIds[0]];
		for (auto fti : forceTargetIds)
			forceTargets.push_back(vec[fti]);
	}
}

void Protocol::openCsvLog()
{
	// check if open
	if (trialLogCsv.is_open()) {
		logError("Tried opening an open csv log file.");
		return;
	}

	string filename = params.session_log_filename;
	// check if file exists
	while (experimental::filesystem::exists(filename)) {
		// adjust it to have another name: filename(NUMBER).csv
		experimental::filesystem::path filepath(filename);

		string ext = filepath.extension().string();
		string basename = filepath.filename().string();
		// replace_extension does not remove the '.'
		if (!ext.empty())
			basename = basename.substr(0, basename.size() - ext.size());

		// check if the name already has a number in the name
		regex filenumber_regex("\\([0-9]+\\)$");
		smatch filenumber_smatch;
		int filenumber = 0;
		if (regex_search(basename, filenumber_smatch, filenumber_regex)) {
			basename = regex_replace(basename, filenumber_regex, "");
			string filenumber_s = filenumber_smatch[0].str();
			filenumber_s = filenumber_s.substr(1, filenumber_s.size() - 2); // remove parentheses
			try
			{
				filenumber = stoi(filenumber_s);
			}
			catch (const std::exception&)
			{
				string buf = "Problems extracting file number from " + filename + ".";
				logWarning(buf.c_str());
			}
		}
		filenumber++;

		basename = basename + "(" + to_string(filenumber) + ")" + ext;
		filepath.replace_filename(basename);
		filename = filepath.string();
	}
	params.session_log_filename = filename.c_str();

	// open file
	trialLogCsv.open(params.session_log_filename, ofstream::out);

	// write the first line - header with all exported columns
	trialLogCsv << "trial_num,"; // As of 5/9/24 This is set to zero on start protocol and is incremented with every trial run.
	trialLogCsv << "repeating_trial,";
	trialLogCsv << "reward,";
    trialLogCsv << "trial_start_time(ms),";
	trialLogCsv << "log_sent_config_to_cameras(ms),";
	trialLogCsv << "object_in_position_time(ms),";
    trialLogCsv << "arm_liftoff_time(ms),";
    trialLogCsv << "log_started_camera_recording(ms),";
    trialLogCsv << "log_started_ps_recording(ms),";
    trialLogCsv << "log_started_ephys_recording(ms),";
	trialLogCsv << "log_sent_start_sync_messages(ms),";
	trialLogCsv << "log_started_monitoring_ps(ms),";
	trialLogCsv << "started_touching_time(ms),";
	for (size_t i_force = 1; i_force < forceTargetIds.size(); i_force++)
	{
		trialLogCsv << "started_touching_time_" << i_force+1 << "(ms), ";
	}
	for (size_t i_force = 1; i_force < forceTargetIds.size(); i_force++)
	{
		trialLogCsv << "force_target_start_time_" << i_force + 1 << "(ms), ";
	}
    trialLogCsv << "arm_return_time(ms),";
    trialLogCsv << "trial_end_time(ms),";
    trialLogCsv << "log_starting_finishing_recordings(ms),";
	trialLogCsv << "log_sent_end_sync_messages(ms),";
    trialLogCsv << "log_stopped_camera_recordings(ms),";
	trialLogCsv << "log_stopped_ps_recordings(ms),";
	trialLogCsv << "trial_finished_time(ms),";
	trialLogCsv << "pos_translation_z(mm),";
	trialLogCsv << "pos_tilt(deg),";
	trialLogCsv << "pos_aperture(mm),";
	trialLogCsv << "targetForce(N),";
    trialLogCsv << "waterDuration(ms),";
    trialLogCsv << "holdDuratation(ms),";
    trialLogCsv << "rewardOnReturn,";
	for (size_t i_force = 1; i_force < forceTargetIds.size(); i_force++)
	{
		trialLogCsv << "targetForce_" << i_force + 1 << "(N), ";
	}
	trialLogCsv << "targetForceRelRangeMin(N),";
	trialLogCsv << "targetForceRelRangeMax(N),";
	trialLogCsv << endl;

}

void Protocol::addLineToCsvLog(
	const bool got_reward,
	const bool repeating,
	const long long trial_start_time,
	const long long log_sent_config_to_cameras,
	const long long object_in_position_time,
	const long long arm_liftoff_time,
    const long long log_started_camera_recording, const long long log_started_ps_recording,
    const long long log_started_ephys_recording, const long long log_sent_start_sync_messages,
	const long long log_started_monitoring_ps,
	const long long trial_end_time,
    const long long log_starting_finishing_recordings, const long long log_sent_end_sync_messages,
	const long long log_stopped_camera_recordings, const long long log_stopped_ps_recordings,
    const long long trial_finished_time,
    const long long arm_return_time)
{
	if (!trialLogCsv.is_open()) {
		logError("Trying to write into a closed log.");
		return;
	}

	// process multi-grasp data
	trialLogCsv << params.counter << ",";                      // "counter,";
	trialLogCsv << (int)repeating << ",";			           // "repeating_trial,";
	trialLogCsv << (int)got_reward << ",";			           // "reward,";
	trialLogCsv << trial_start_time << ",";			           // "trial_start_time(ms),";
    trialLogCsv << log_sent_config_to_cameras << ",";          // "log_sent_config_to_cameras(ms),";
	trialLogCsv << object_in_position_time << ",";	           // "object_in_position_time(ms),";
	trialLogCsv << arm_liftoff_time << ",";			           // "arm_liftoff_time(ms),";
    trialLogCsv << log_started_camera_recording << ",";        // "log_started_camera_recording(ms),";
    trialLogCsv << log_started_ps_recording << ",";            // "log_started_ps_recording(ms),";
    trialLogCsv << log_started_ephys_recording << ",";         // "log_started_ephys_recording(ms),";
	trialLogCsv << log_sent_start_sync_messages << ",";        // "log_sent_start_sync_messages(ms),";
    trialLogCsv << log_started_monitoring_ps << ",";           // "log_started_monitoring_ps(ms),";

	// process multi-grasp data
	for (size_t i_force = 0; i_force < forceTargetIds.size(); i_force++)
	{
		// started_touching_time(ms), ";
		// or
		// "started_touching_time_" << i_force + 1 << "(ms), ";
		if (i_force < m_started_touching_times.size())
			trialLogCsv << m_started_touching_times[i_force] << ",";
		else
			trialLogCsv << "0,";
	}
	// this one only if there are more than 1 grasps expected
	for (size_t i_force = 1; i_force < forceTargetIds.size(); i_force++)
	{
		// "force_target_start_time_" << i_force + 1 << "(ms), ";
		if (i_force - 1 < m_force_target_start_times.size())
			trialLogCsv << m_force_target_start_times[i_force - 1] << ",";
		else
			trialLogCsv << "0,";
	}

    trialLogCsv << arm_return_time << ",";                     // "arm_return_time(ms),";
	trialLogCsv << trial_end_time << ",";			           // "trial_end_time(ms),";
    trialLogCsv << log_starting_finishing_recordings << ",";   // "log_starting_finishing_recordings(ms),";
	trialLogCsv << log_sent_end_sync_messages << ",";		   // "log_sent_end_sync_messages(ms),";
    trialLogCsv << log_stopped_camera_recordings << ",";       // "log_stopped_camera_recordings(ms),";
    trialLogCsv << log_stopped_ps_recordings << ",";           // "log_stopped_ps_recordings(ms),";
	trialLogCsv << trial_finished_time << ",";		           // "trial_finished_time(ms),";
	trialLogCsv << params.pos_translation_x << ",";	           // "pos_translation_x(mm),";
	trialLogCsv << params.pos_tilt << ",";			           // "pos_tilt(deg),";
	trialLogCsv << params.pos_aperture << ",";		           // "pos_aperture(mm),";
	trialLogCsv << params.targetForce << ",";		           // "targetForce(N),";
    trialLogCsv << params.rewardDuration << ",";               //  "waterDuration(ms),";
    trialLogCsv << params.thresholdPeriod * 1000 << ",";       //  "holdDuratation(ms),";
    trialLogCsv << reward_on_return.load() << ",";             //  "rewardOnReturn,";

	for (size_t i_force = 1; i_force < forceTargets.size(); i_force++)
	{
		trialLogCsv << forceTargets[i_force] << ",";
	}
	trialLogCsv << params.targetForceRelRangeMin << ",";	   // "targetForceRelRangeMin(N),";
	trialLogCsv << params.targetForceRelRangeMax << ",";	   // "targetForceRelRangeMax(N),";
	trialLogCsv << endl;
}

void Protocol::closeCsvLog()
{
	if (trialLogCsv.is_open())
		trialLogCsv.close();
}

void Protocol::trialStateGuiUpdate()
{
	switch (protocolState)
	{
	case ProtocolState::shutdown:
		m_trialStatus->SetWindowTextA("SHUTDOWN");
		break;
	case ProtocolState::initializing:
		m_trialStatus->SetWindowTextA("INITIALIZING");
		break;
	case ProtocolState::trialReady:
		m_trialStatus->SetWindowTextA("READY");
		break;
	case ProtocolState::trialInProgress:
		m_trialStatus->SetWindowTextA("IN PROGRESS");
		break;
	case ProtocolState::trialFinalizing:
		m_trialStatus->SetWindowTextA("FINALIZING");
		break;
	case ProtocolState::shuttingDown:
		m_trialStatus->SetWindowTextA("SHUTTING DOWN");
		break;
	default:
		m_trialStatus->SetWindowTextA("UNKNOWN-ERROR");
		break;
	}
}

void Protocol::autoLoopToggle(const bool enable)
{
	loopAutomatically = enable;
	loopChk->SetCheck((int)enable);
}

void Protocol::initDevices()
{
	// NI card: photoresistors, motor, reward and ephys
	m_NIUsb6001card.config();

	// motor
	if (motorHub) {
		logWarning("Motor Hub already initialized, cannot init again.");
	}
	else {
		motorHub = new TeknicMotorApi(params.motors_motors_filename, params.motors_axes_filename);
	}

    if(armClient)
        logWarning("Arm Client already initialized, cannot init again");
    else
        armClient = new KinovaArmClient();

	// LEDs
	if (ledStrip) {
		logWarning("Led strips already initialized, cannot init again.");
	}
	else {
		ledStrip = new LedStrip(params.leds_com_port, params.leds_comPortFriendlyName);
        ledStrip->top_stripe_red = params.leds_top_stripe_color_red;
        ledStrip->top_stripe_green = params.leds_top_stripe_color_green;
        ledStrip->top_stripe_blue = params.leds_top_stripe_color_blue;
		ledStrip->top_brightness = params.leds_top_stripe_brightness;
		ledStrip->top_reverse_order = (bool) params.leds_top_stripe_reverse_order;
        ledStrip->bottom_stripe_red = params.leds_bottom_stripe_color_red;
        ledStrip->bottom_stripe_green = params.leds_bottom_stripe_color_green;
        ledStrip->bottom_stripe_blue = params.leds_bottom_stripe_color_blue;
		ledStrip->bottom_brightness = params.leds_bottom_stripe_brightness;
		ledStrip->bottom_reverse_order = (bool) params.leds_bottom_stripe_reverse_order;
		if (params.leds_number > 0)
			ledStrip->num_leds_per_strip = (unsigned int) params.leds_number;
		if (ledStrip->wasInitializedCorrectly() && params.leds_run_test) {
			logInfo("Starting LED test.");
			ledStrip->test();
			logInfo("Finished LED test.");
		}
	}
}

void Protocol::releaseDevices()
{
	// NI card
	m_NIUsb6001card.stop();

	// motor
	if (motorHub) {  // check if nullptr
		delete motorHub;
		motorHub = nullptr;
	}

    if(armClient)
    {
        delete armClient;
        armClient = nullptr;
    }

	// LEDs
	if (ledStrip) {  // check if nullptr
		delete ledStrip;
		ledStrip = nullptr;
	}
}

bool Protocol::isMotorsOn()
{
	return motorHub->wasInitializedCorrectly();
}

bool Protocol::isRewardOn()
{
	return m_NIUsb6001card.wasInitializedCorrectly();
}

bool Protocol::isLightSensorsOn()
{
	// TODO split NI USB card intialization into specific ones
	// return (use_front_light_sensor.load() || use_rear_light_sensor.load()) && m_NIUsb6001card.wasInitializedCorrectly();
	return m_NIUsb6001card.wasInitializedCorrectly();
}

bool Protocol::isEphysOn()
{
	return m_NIUsb6001card.wasInitializedCorrectly();
}

bool Protocol::isLedsOn()
{
	return ledStrip->wasInitializedCorrectly();
}

void Protocol::watch_early_grab()
{
	std::chrono::steady_clock::time_point* startTime = nullptr;
	atomic<double> leftForce = 0;
	atomic<double> rightForce = 0;
	while (!stopWatch) {
		// see if monkey lifted arm
		// if (isLightSensorsOn() && !isArmAtRest()) {
		if (isLightSensorsOn()) {
			if ((use_front_light_sensor.load() && !IS_FRONT_PHOTORESISTOR_COVERED.load()) ||
				(use_rear_light_sensor.load() && !IS_REAR_PHOTORESISTOR_COVERED.load()) ||
				(use_right_arm_touch.load() && !IS_RIGHT_ARMSENSOR_TOUCHED.load()) ||
				(use_left_arm_touch.load() && !IS_LEFT_ARMSENSOR_TOUCHED.load())) {

				if (!startTime) {
					// jsut started
					startTime = new auto(Times::getCurrentTime());
				}

			}
			else {
				startTime = nullptr;
			}
		}

		if (startTime && Times::isTimeout(*startTime, params.photoresistor_status_switch_delay / 1000)) {
			stopTrial.store(true);
			stop_motors();
			break;
		}

		// ask pressure sensor for pressure
		if (m_touchSensorClient.isConnected()) {
			m_touchSensorClient.getForce(&leftForce, &rightForce);

			if (leftForce + rightForce > params.minimalTouchForce) {
				stopTrial.store(true);
				stop_motors();
				break;
			}
		}
	}
}

bool Protocol::is_arm_at_rest()
{
	// sensors not found, or ignore them both - skip this whole function
	if (!isLightSensorsOn() || !(use_front_light_sensor.load() || use_rear_light_sensor.load() || use_right_arm_touch.load() || use_left_arm_touch.load() )) {
		return true;
	}
	/*
	char tmp[256];
	sprintf(tmp, "status of sensors:\n\
		use left_light: %i ;; is_covered: %i\n\
		use right_ight : % i;; is_covered: %i\n\
		use left_touch: %i ;; is_covered: %i\n\
		use right_touch : % i;; is_covered: %i\n",
		use_front_light_sensor.load(), IS_FRONT_PHOTORESISTOR_COVERED.load(),
		use_rear_light_sensor.load(), IS_REAR_PHOTORESISTOR_COVERED.load(),
		use_right_arm_touch.load(), IS_RIGHT_ARMSENSOR_TOUCHED.load(),
		use_left_arm_touch.load(), IS_LEFT_ARMSENSOR_TOUCHED.load());
	logInfo(tmp);
	*/
	if ((use_front_light_sensor.load() && !IS_FRONT_PHOTORESISTOR_COVERED.load()) ||
		(use_rear_light_sensor.load() && !IS_REAR_PHOTORESISTOR_COVERED.load()) ||
        (use_right_arm_touch.load() && !IS_RIGHT_ARMSENSOR_TOUCHED.load()) ||
        (use_left_arm_touch.load() && !IS_LEFT_ARMSENSOR_TOUCHED.load()))
        {
		    // uncovered
		    if (_arm_at_rest_start_time) {
			    delete _arm_at_rest_start_time;
			    _arm_at_rest_start_time = nullptr;
		    }
	    }
	else {
		// just started covering
		if (!_arm_at_rest_start_time)
			_arm_at_rest_start_time = new auto(Times::getCurrentTime());
	}

	if (_arm_at_rest_start_time &&
		Times::getElapsedMilliSecsSince(*_arm_at_rest_start_time) > params.photoresistor_status_switch_delay) {
		return true;
	}

	return false;
}

void Protocol::prepare_to_monitor_arm_at_rest()
{
	if (_arm_at_rest_start_time) {
		delete _arm_at_rest_start_time;
		_arm_at_rest_start_time = nullptr;
	}
}

int Protocol::wait_until_arm_at_rest()
{
	int flag = 0;
	// sensors not found, or ignore them both - skip this whole function
	if (!isLightSensorsOn() || !(use_front_light_sensor.load() || use_rear_light_sensor.load())) {
		return flag;
	}

	auto waitStart = Times::getCurrentTime();
	std::chrono::steady_clock::time_point* startTime = nullptr;
	double timeout = 20 * 60; // seconds

	while (true) {
		if ((use_front_light_sensor.load() && !IS_FRONT_PHOTORESISTOR_COVERED) ||
			(use_rear_light_sensor.load() && !IS_REAR_PHOTORESISTOR_COVERED)) {
			// uncovered
			if (startTime) {
				delete startTime;
				startTime = nullptr;
			}
		}
		else {
			// just started covering
			if (!startTime)
				startTime = new auto(Times::getCurrentTime());
		}

		if (startTime && Times::getElapsedMilliSecsSince(*startTime) > params.photoresistor_status_switch_delay) {
			break;
		}


		if (Times::isTimeout(waitStart, timeout)) {
			flag = -1;
			break;
		}
	}
	return flag;
}

int Protocol::wait_until_arm_liftoff()
{
	// NOT USED
	int flag = 0;
	// sensors not found, or ignore them both - skip this whole function
	if (!isLightSensorsOn() || !(use_front_light_sensor.load() || use_rear_light_sensor.load())) {
		return flag;
	}

	auto waitStart = Times::getCurrentTime();
	double timeout = 5 * 60; // seconds

	while (true) {
		if (use_front_light_sensor.load() && use_rear_light_sensor.load()) {
			if (!IS_FRONT_PHOTORESISTOR_COVERED && !IS_REAR_PHOTORESISTOR_COVERED)
				break;
		}
		else if (use_front_light_sensor.load()) {
			if (!IS_FRONT_PHOTORESISTOR_COVERED)
				break;
		}
		else {
			if (!IS_REAR_PHOTORESISTOR_COVERED)
				break;
		}
		//if (!isLightSensorsOn() || !isArmAtRest()) {
		//	break;
		//}
		if (Times::isTimeout(waitStart, timeout)) {
			flag = -1;
			break;
		}
	}
	return flag;
}


void Protocol::armMonitoringThread()
{
    int offCounter = 0;
	char msg[256];
	sprintf(msg, "This is the background thread for passive, allowInterupt: %i, monitorPassive: %i", allowInterupt.load() == true, monitor_passive_arm.load() == true);
	logInfo(msg);
	int totCount = 0;
    while(allowInterupt.load() && monitor_passive_arm.load())
    {
		totCount++;
        if(!IS_LEFT_ARMSENSOR_TOUCHED && which_passive_arm == -1)
            offCounter += 1;
        else if(!IS_RIGHT_ARMSENSOR_TOUCHED && which_passive_arm == 1)
            offCounter += 1;
        else
            offCounter = 0;
		if (offCounter >= 25)
		{
			logInfo("Exceeded time off from armrest, arborting trial");
			stop_motors();
			stopTrial.store(true);
			offCounter = 0;
			allowInterupt.store(false);
		}

    }
	sprintf(msg, "Finished loop for passive, totcount: %i, allowInterupt: %i, monitorPassive: %i", totCount, allowInterupt.load() == true, monitor_passive_arm.load() == true);
	logInfo(msg);
    return;
}

void Protocol::start_ephys_recording()
{
	m_NIUsb6001card.ephysSyncStart();
}

void Protocol::break_ephys_recording()
{
	m_NIUsb6001card.ephysSyncStop();
}

/// <summary>
/// Asks Pressure Sensor if the reward has been earned, ends when sets m_earnedReward to true
/// Pressure sensor constantly returns whether the grab is occuring along with the success of the trial,
/// and if so happens before the grasp do not give reward
/// </summary>
void Protocol::m_asyncTrialConditionMonitor()
{
	m_earnedReward = false;
	m_stopAsyncTrialConditionMonitor = false;

	atomic<int> result;
	atomic<double> leftForce = 0;
	atomic<double> rightForce = 0;
	double totalForce = 0;

	// unchanging definitions (per force target)
	std::deque<double> additionalTargetForces(forceTargets);
	if (!additionalTargetForces.empty())  // the first one is always used from params - connected to GUI
		additionalTargetForces.pop_front();
	double targetForce = params.targetForce;
	double targetForceMin = std::max(targetForce + params.targetForceRelRangeMin, params.targetForceTotalMinThreshold);
	double targetForceMax = targetForce + params.targetForceRelRangeMax;
	double proportionForceMin = targetForce * params.thresholdForceEachProportion;
	long thresholdPeriodMicrosecs = Times::secToMicrosecs(params.thresholdPeriod);

	// target force range
	if (!m_stopAsyncTrialConditionMonitor && isLedsOn() && m_touchSensorClient.isConnected()) {
		ledStrip->set_top_stripe_lights(
			targetForceMin / params.targetForceTotalMax,
			targetForceMax / params.targetForceTotalMax);
	}

	// additional forces
	m_force_target_start_times.clear();
	int i_force = 0;
	m_started_touching_times.clear();
	m_started_touching_times.push_back(0);

	// tracking the touching period
	std::chrono::steady_clock::time_point* startTime = nullptr;

	while (!m_stopAsyncTrialConditionMonitor) {
		if (m_touchSensorClient.isConnected()) {
			// ask touch sensor for the force on each plate
			m_touchSensorClient.getForce(&leftForce, &rightForce);
			totalForce = leftForce + rightForce;

			// update the visualized force
			if (isLedsOn()) {
				ledStrip->set_bottom_stripe_lights(totalForce / params.targetForceTotalMax);
			}

			// check if touching now and keep time of touch start
			// minimum force level of 0.2 of desired and total excedes the desired
			if (totalForce >= targetForceMin && totalForce <= targetForceMax &&
				leftForce >= proportionForceMin && rightForce >= proportionForceMin) {
				if (!startTime) { // just started touching
					startTime = new auto(Times::getCurrentTime());
					m_started_touching_times[i_force] = Times::getCurrentTimeInMilliSecs();
					logInfo("Started touching.");
				}
			}
			else { // not touching
				if (startTime)
					startTime = nullptr;
			}

			// check if touching for long enough
			if (startTime)
				if (Times::getElapsedMicroSecsSince(*startTime) > thresholdPeriodMicrosecs)
					result = 1;

			// exit if result is successfull
			// or move to the next target force
			if (result > 0) {
				if (additionalTargetForces.size() == 0) {
					m_earnedReward = true;
					m_stopAsyncTrialConditionMonitor = true;
					logInfo("Touching successfull.");
				}
				else {
					logInfo("Going to next force level.");

					// pull the new force
					targetForce = additionalTargetForces.front();
					additionalTargetForces.pop_front();  // clean

					// and reset the variables
					targetForceMin = std::max(targetForce + params.targetForceRelRangeMin, params.targetForceTotalMinThreshold);
					targetForceMax = targetForce + params.targetForceRelRangeMax;
					proportionForceMin = targetForce * params.thresholdForceEachProportion;

					// change target force
					if (isLedsOn()) {
						ledStrip->set_top_stripe_lights(
							targetForceMin / params.targetForceTotalMax,
							targetForceMax / params.targetForceTotalMax);
					}

					// record the time of switch and increase the counter
					m_force_target_start_times.push_back(Times::getCurrentTimeInMilliSecs());
					m_started_touching_times.push_back(0);
					i_force++;

					// restart the time counter
					startTime = nullptr;
					result = 0;
				}
			}
		}
		else {  // no reason to run if no sensor connected
			m_stopAsyncTrialConditionMonitor = true;
		}
	}
}
