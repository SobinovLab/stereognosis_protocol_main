#include "ProtocolParameters.h"

using namespace std;

CString ProtocolParameters::try_finding_session_csv()
{
	string fname;

	if (Folders::find_latest_csv(default_session_file_directory, fname)) {
		return CString(fname.c_str());
	}

	if (Folders::find_latest_csv("./App/session_configs", fname)) {  // debugging
		return CString(fname.c_str());
	}

	return "";
}

CString ProtocolParameters::make_log_filename()
{
	string file_basename;
	if (session_filename.GetLength() == 0) {
		file_basename = "session_" + Times::getFormattedDateTime() + ".csv";
	}
	else {
		file_basename = experimental::filesystem::path(string(session_filename).c_str()).filename().string();
		file_basename = "session_ " + Times::getFormattedDateTime() + "_from_" + 
			file_basename.substr(0, file_basename.size() - 4) + ".csv";
	}

	file_basename = default_session_log_directory + file_basename;

	return file_basename.c_str();
}

ProtocolParameters::ProtocolParameters()
{
	init();
}

ProtocolParameters::~ProtocolParameters()
{
}

void ProtocolParameters::init()
{
	// protocol
	maxWaitTime = 20;  // sec
	intertrialWaitTime = 2; // sec
	session_filename = try_finding_session_csv();
	session_log_filename = make_log_filename();
	rewardDuration = 1000;

	// trial
	trial_number = 0;
	total_trials = 0;
	pos_translation_x = 115;  // see MotorAPI
	pos_tilt = 0;
	pos_aperture = 0;

	// camera servers
	cs_ip1 = "205.208.87.188";
	cs_port1 = 63874;
	cs_ip2 = "205.208.63.128";
	cs_port2 = 63874;

	// camera config
	cs_framerate = 50;
	cs_recordingPeriod = 60;
	cs_refSerial = 19340298;
	cs_exposure = "2500";
	cs_gain = "20";
	cs_capture_n_frames = 10;

	// pressure sensor server
	tss_ip = "205.208.87.188";
	tss_port = 54940;

	// pressure sensor config
	targetForce = 3;
	targetForceRelRangeMin = -2;
	targetForceRelRangeMax = 2;
	targetForceTotalMinThreshold = 0.5;
	thresholdPeriod = 2;  // seconds
	thresholdForceEachProportion = 0.2;
	minimalTouchForce = 0.5;

}
