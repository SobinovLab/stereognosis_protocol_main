#include "Protocol.h"

using namespace std;

constexpr auto PRECISION = "%03d";


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
	return motorHub->wereHomed();
}

TEKNIC_MOTOR_API_CODE Protocol::home_motors()
{
	return motorHub->home();
}

void Protocol::stop_motors()
{
	motorHub->stop();
}

TEKNIC_MOTOR_API_CODE Protocol::motors_neutral_position()
{
	return motorHub->neutral_position();
}

void Protocol::connect_camera_client1()
{
	if (m_cameraClient1.isConnected()) {
		// warning?
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
		// warning?
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
	return start_camera_recording(params.trial_number);
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

void Protocol::sync_message_trial_start()
{
	if (m_cameraClient1.isConnected()) {
		m_cameraClient1.syncMessageTrialStart();
	}
	if (m_cameraClient2.isConnected()) {
		m_cameraClient2.syncMessageTrialStart();
	}

	if (m_touchSensorClient.isConnected()) {
		m_touchSensorClient.syncMessageTrialStart();
	}
}

void Protocol::sync_message_trial_end()
{
	if (m_cameraClient1.isConnected()) {
		m_cameraClient1.syncMessageTrialEnd();
	}
	if (m_cameraClient2.isConnected()) {
		m_cameraClient2.syncMessageTrialEnd();
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
	start_pressure_sensor_recording(params.trial_number);
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

void Protocol::watch_early_grab()
{
	atomic<double> leftForce = 0;
	atomic<double> rightForce = 0;
	while (!stopWatch) {
		// see if monkey lifted arm
		if (isLightSensorsOn() && !isArmAtRest()) {
			stopTrial = true;
			stop_motors();
			break;
		}

		// ask pressure sensor for pressure
		if (m_touchSensorClient.isConnected()) {
			m_touchSensorClient.getForce(&leftForce, &rightForce);

			if (leftForce + rightForce > params.minimalTouchForce) {
				stopTrial = true;
				stop_motors();
				break;
			}
		}
	}
}

void Protocol::run()
{
	this->stopProtocol.store(false);
	setCurrentState(ProtocolState::initializing); // display the state of the trial on the GUI
	int rets = 0;
	TEKNIC_MOTOR_API_CODE motor_rets = TEKNIC_MOTOR_API_CODE::OK;

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
	}
	else {
		usingLoadedSession = true;
		params.total_trials = session_values.size();
		for (size_t i = 0; i < params.total_trials; i++)
			repeating_trial.push_back(false);
	}

	// on first start, looping should be disabled until the start trial button is pressed
	autoLoopToggle(true);
	loopAutomatically = false;

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
    long long log_starting_finishing_recordings;
	long long log_sent_end_sync_messages;
    long long log_stopped_camera_recordings;
    long long log_stopped_ps_recordings;
	long long trial_finished_time;

	// class member variable
	params.trial_number = 0;
	int preset_trial_number;
	auto intertrialWaitStartTime = Times::getCurrentTime();  // set at the end of the previous iteration
	auto trialStartTime = Times::getCurrentTime();  // set when the object is in position

	// Run the protocol loop
	while (!this->stopProtocol.load())
	{
		// ---------------- Preparing trial
		trial_start_time = 0;
        log_sent_config_to_cameras = 0;
		object_in_position_time = 0;
		arm_liftoff_time = 0;
        log_started_camera_recording = 0;
        log_started_ps_recording = 0;
        log_started_ephys_recording = 0;
		log_sent_start_sync_messages = 0;
        log_started_monitoring_ps = 0;
		m_startedTouchingTime = 0;
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

		// wait until arm at rest
		if (wait_until_arm_at_rest() < 0) {
			// timeout for 20 mins
			stopTrial = true;
			stopProtocol = true;
		}

		// waiting for the start of the next trial
		while (!this->startTrial.load() &&
			!this->stopProtocol.load() &&
			!(loopAutomatically.load() && Times::isTimeout(intertrialWaitStartTime, params.intertrialWaitTime))) {
			// waiting for:
			//	Start trial button to be pressed
			//  stop of protocol
			//  if looping is selected, timeout of intertrial time
		}

		// The usual place to exit the protocol, if not at the end of a trial
		if (stopProtocol)
			break;

		// Default behavior is looping - after the first trial
		autoLoopToggle(true);

		// GUI Can't click on StartTrial anymore
		trialFieldsEnableStart(false);

		// just in case any parameters changed, pull from GUI
		pull_variables_from_gui();

		// if the trial changed, load it instead
		if (usingLoadedSession && preset_trial_number != params.trial_number && params.trial_number < params.total_trials) {
			matchLoadedSessionTrialToParams(session_line1, session_line2, session_values[params.trial_number]);
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

		// start a thread that asks if monkey grabbed and stopTrial=1 if it did
		stopWatch = false;
		thread watchThread(&Protocol::watch_early_grab, this);

		// motor movement - this thread will be locked, can be interrupted
		// The motors API only supports position control as dynamics are not important
		// TODO make gui list-based
		vector<string> axes = { "translation_X", "tilt", "aperture" };
		vector<double> positions = { params.pos_translation_x, params.pos_tilt, params.pos_aperture };
		rets = 0;
		motor_rets = TEKNIC_MOTOR_API_CODE::OK;
		if (!stopTrial)
			motor_rets = motorHub->preshape(axes, positions);
		if (TeknicMotorApi::isError(motor_rets))
		{
			// bad error
			if (motor_rets != TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR && 
				motor_rets != TEKNIC_MOTOR_API_CODE::USER_INTERRUPT) {
				logError("Bad motor error encountered during preshape. Interrupting the protocol.");
				rets = -1;
			}
			else if (motor_rets == TEKNIC_MOTOR_API_CODE::USER_INTERRUPT) {
				// nothing, since it can be triggered by the watchThread
				logWarning("User or early grab motor interrupt during preshape.");
			}
		}

		// start recordings
		start_camera_recording();  // TODO process it?
		log_started_camera_recording = Times::getCurrentTimeInMilliSecs();
		start_pressure_sensor_recording();
		log_started_ps_recording = Times::getCurrentTimeInMilliSecs();

		// approach
		if (!rets && !stopTrial) {
			motor_rets = motorHub->approach(axes, positions);
			if (TeknicMotorApi::isError(motor_rets))
			{
				if (motor_rets == TEKNIC_MOTOR_API_CODE::USER_INTERRUPT) {
					// nothing, since it can be triggered by the watchThread
				}
				else if (motor_rets != TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR) {
					// bad error
					logError("Bad motor error encountered during approach. Interrupting the protocol.");
					rets = -1;
				}
			}
		}

		// log
		if (!stopTrial) 
			object_in_position_time = Times::getCurrentTimeInMilliSecs();

		// stop the watch thread
		stopWatch = true;
		watchThread.join();

		// if the trial was not interrupted, wait for the hand liftoff
		if (!stopTrial && !rets) {
			// TODO in the current structure does not make sense
			//rets = wait_until_arm_liftoff();

			arm_liftoff_time = Times::getCurrentTimeInMilliSecs();
		}

		// if the motors made it successfully to the final position and the animal has lifted the arm
		if (!stopTrial && rets >= 0) {
			// spawn the process that monitors the async stopping conditions
			m_asyncTrialSuccessMonitorThread = new thread(&Protocol::m_asyncTrialConditionMonitor, this);
            log_started_monitoring_ps = Times::getCurrentTimeInMilliSecs();

			trialStartTime = Times::getCurrentTime();
			Sounds::playStartTaskTone();

			// wait for stop trial signal from interface, success from the monitor thread, or timeout
			while (!this->stopTrial.load() &&
				!this->m_earnedReward.load() &&
				!Times::isTimeout(trialStartTime, params.maxWaitTime)) {
			}

			// if stop trial button was pressed, turn off the loop - it is reenable automatically in the beginning of trial
			if (this->stopTrial.load())
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

		// GUI Cannot interrupt the rest of trial
		trialFieldsEnableRetreat(false);

		// ---------------- Finishing trial
		setCurrentState(ProtocolState::trialFinalizing);

		// log
		trial_end_time = Times::getCurrentTimeInMilliSecs();

		// Give the reward or not
		if (m_earnedReward || deservesReward) {
			reward();
		}
		else {
			Sounds::playErrorTone();

			// append the failed trial to the end
			if (usingLoadedSession) {
				session_values.push_back(session_values[params.trial_number]);
				repeating_trial.push_back(true);
			}
		}

		// retreat motors
		motorHub->retreat();
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
		addLineToCsvLog(m_earnedReward || deservesReward, repeating_trial[params.trial_number],
			trial_start_time, log_sent_config_to_cameras, object_in_position_time,
            arm_liftoff_time,
            log_started_camera_recording, log_started_ps_recording,
            log_started_ephys_recording, log_sent_start_sync_messages,
			log_started_monitoring_ps,
            trial_end_time,
            log_starting_finishing_recordings, log_sent_end_sync_messages,
			log_stopped_camera_recordings, log_stopped_ps_recordings,
            trial_finished_time);


		// wait for the signal from recording devices that the data has been saved - is Ready
		rets = wait_for_cameras_finish_saving();
		if (rets < 0) {
			AfxMessageBox("Cameras are taking too long to save the data. Stopping the protocol.");
			stopTrial = true;
			stopProtocol = true;
		}

		params.trial_number++;
	}
	setCurrentState(ProtocolState::shuttingDown);

	trialFieldsEnableStart(false);
	trialFieldsEnableRetreat(false);

	// retreat motors
	motorHub->retreat();

	// release all devices in the destructor
	closeCsvLog();

	setCurrentState(ProtocolState::shutdown);
}

void Protocol::set_photoresistor_monitors(CStaticColor* front, CStaticColor* rear)
{
	// does not care if the card is there or whatever
	m_NIUsb6001card.setFrontPhotoresistorMonitor(front);
	m_NIUsb6001card.setRearPhotoresistorMonitor(rear);
}

void Protocol::set_camera1_gui_controls(CEdit* serverLogCtrl)
{
	m_cameraClient1.clientLogGuiEdt = serverLogCtrl;
}

void Protocol::set_camera2_gui_controls(CEdit* serverLogCtrl)
{
	m_cameraClient2.clientLogGuiEdt = serverLogCtrl;
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
	for (size_t i = 0; i < line1.size(); i++)
	{
		axis = line1[i];
		deriv = line2[i];

		// TODO make list-based GUI
		if (axis == "translation_X") {
			if (deriv == "position") {
				params.pos_translation_x = vec[i];
			}
		}

		if (axis == "tilt") {
			if (deriv == "position") {
				params.pos_tilt = vec[i];
			}
		}

		if (axis == "aperture") {
			if (deriv == "position") {
				params.pos_aperture = vec[i];
			}
		}

		if (axis == "total_force") {
			if (deriv == "position") {
				params.targetForce = vec[i];
			}
		}
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
	trialLogCsv << "trial_num,";
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
    trialLogCsv << "trial_end_time(ms),";
    trialLogCsv << "log_starting_finishing_recordings(ms),";
	trialLogCsv << "log_sent_end_sync_messages(ms),";
    trialLogCsv << "log_stopped_camera_recordings(ms),";
	trialLogCsv << "log_stopped_ps_recordings(ms),";
	trialLogCsv << "trial_finished_time(ms),";
	trialLogCsv << "pos_translation_z(mm),";
	trialLogCsv << "pos_tilt(deg),";
	trialLogCsv << "pos_aperture(mm),";
	trialLogCsv << endl;

}

void Protocol::addLineToCsvLog(const bool got_reward, const bool repeating,
	const long long trial_start_time, const long long log_sent_config_to_cameras, const long long object_in_position_time,
	const long long arm_liftoff_time,
    const long long log_started_camera_recording, const long long log_started_ps_recording,
    const long long log_started_ephys_recording, const long long log_sent_start_sync_messages,
	const long long log_started_monitoring_ps,
	const long long trial_end_time,
    const long long log_starting_finishing_recordings, const long long log_sent_end_sync_messages,
	const long long log_stopped_camera_recordings, const long long log_stopped_ps_recordings,
    const long long trial_finished_time)
{
	if (!trialLogCsv.is_open()) {
		logError("Trying to write into a closed log.");
		return;
	}

	trialLogCsv << params.trial_number << ",";                 // "trial_num,";
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
	if (got_reward)
		trialLogCsv << m_startedTouchingTime << ","; // started_touching_time(ms), ";
	else
		trialLogCsv << "0,"; // started_touching_time(ms), ";
	trialLogCsv << trial_end_time << ",";			           // "trial_end_time(ms),";
    trialLogCsv << log_starting_finishing_recordings << ",";   // "log_starting_finishing_recordings(ms),";
	trialLogCsv << log_sent_end_sync_messages << ",";		   // "log_sent_end_sync_messages(ms),";
    trialLogCsv << log_stopped_camera_recordings << ",";       // "log_stopped_camera_recordings(ms),";
    trialLogCsv << log_stopped_ps_recordings << ",";           // "log_stopped_ps_recordings(ms),";
	trialLogCsv << trial_finished_time << ",";		           // "trial_finished_time(ms),";
	trialLogCsv << params.pos_translation_x << ",";	           // "pos_translation_x(mm),";
	trialLogCsv << params.pos_tilt << ",";			           // "pos_tilt(deg),";
	trialLogCsv << params.pos_aperture << ",";		           // "pos_aperture(mm),";
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
		motorHub = new TeknicMotorApi("./configuration/motors_stereognosis1.json", "./configuration/axes_stereognosis.json");
	}

	// LEDs
	if (ledStrip) {
		logWarning("Led strips already initialized, cannot init again.");
	}
	else {
		ledStrip = new LedStrip(params.leds_com_port);
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
		if (ledStrip->wasInitializedCorrectly() && params.leds_run_test) {
			logInfo("Starting LED test.");
			ledStrip->test();
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
	return (use_front_light_sensor.load() || use_rear_light_sensor.load()) && m_NIUsb6001card.wasInitializedCorrectly();
}

bool Protocol::isEphysOn()
{
	return m_NIUsb6001card.wasInitializedCorrectly();
}

bool Protocol::isLedsOn()
{
	return ledStrip->wasInitializedCorrectly();
}

bool Protocol::isArmAtRest()
{
	return ((!use_front_light_sensor.load() || IS_FRONT_PHOTORESISTOR_COVERED) && 
			(!use_rear_light_sensor.load() || IS_REAR_PHOTORESISTOR_COVERED));
}

int Protocol::wait_until_arm_at_rest()
{
	auto waitStart = Times::getCurrentTime();
	double timeout = 20 * 60; // seconds

	int flag = 0;
	while (true) {
		if (!isLightSensorsOn() || isArmAtRest()) {
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
	auto waitStart = Times::getCurrentTime();
	double timeout = 5 * 60; // seconds

	int flag = 0;
	while (true) {
		if (!isLightSensorsOn() || !isArmAtRest()) {
			break;
		}
		if (Times::isTimeout(waitStart, timeout)) {
			flag = -1;
			break;
		}
	}
	return flag;
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
	std::chrono::steady_clock::time_point* startTime = nullptr;

	while (!m_stopAsyncTrialConditionMonitor) {
		if (m_touchSensorClient.isConnected()) {
			// ask touch sensor for the force on each plate
			m_touchSensorClient.getForce(&leftForce, &rightForce);

			// update the visualized force
			if (isLedsOn()) {
				ledStrip->set_top_stripe_lights(
					(params.targetForce + params.targetForceRelRangeMin) / params.targetForceTotalMax,
					(params.targetForce + params.targetForceRelRangeMax) / params.targetForceTotalMax);
				ledStrip->set_bottom_stripe_lights(
					leftForce + rightForce / params.targetForceTotalMax);
			}

			// check if touching now and keep time of touch start
			// minimum force level of 0.2 of desired and total excedes the desired
			if (leftForce + rightForce >= std::max(params.targetForce + params.targetForceRelRangeMin, params.targetForceTotalMinThreshold) &&
				leftForce + rightForce <= params.targetForce + params.targetForceRelRangeMax &&
				leftForce > params.targetForce * params.thresholdForceEachProportion &&
				rightForce > params.targetForce * params.thresholdForceEachProportion) {
				if (!startTime) { // just started touching
					startTime = new auto(Times::getCurrentTime());
					m_startedTouchingTime = chrono::duration_cast<chrono::milliseconds>(startTime->time_since_epoch()).count();
					logInfo("Started touching.");
				}
			}
			else { // not touching
				if (startTime)
					startTime = nullptr;
			}

			// check if touching for long enough
			if (startTime)
				if (Times::getElapsedMicroSecsSince(*startTime) > Times::secToMicrosecs(params.thresholdPeriod))
					result = 1;

			// exit if result is successfull
			if (result > 0) {
				m_earnedReward = true;
				m_stopAsyncTrialConditionMonitor = true;
				logInfo("Touching successfull.");
			}
		}
		else {  // no reason to run if no sensor connected
			m_stopAsyncTrialConditionMonitor = true;
		}

	}
}
