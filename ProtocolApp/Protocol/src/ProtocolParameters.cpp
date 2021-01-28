#include "ProtocolParameters.h"

using namespace std;

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
	session_filename = "";  // TODO
	session_log_filename = "";
	rewardDuration = 1000;

	// trial
	trial_number = 0;
	total_trials = 0;
	pos_translation_z = 115;  // see MotorAPI
	pos_tilt = 0;
	pos_aperture = 0;

	// camera servers
	cs_ip1 = "205.208.87.188";
	cs_port1 = 63874;
	cs_ip2 = "205.208.63.128";
	cs_port2 = 63874;

	// camera config
	cs_framerate = 50;
	cs_recordingPeriod = 25;
	cs_refSerial = 19194009;
	cs_exposure = "2500";
	cs_gain = "20";
	cs_capture_n_frames = 10;

	// pressure sensor server
	tss_ip = "localhost";
	tss_port = 54940;

	// pressure sensor config
	thresholdTotalForce = 50;
	thresholdPeriod = 2;  // seconds
	thresholdForceEachProportion = 0.2;
	minimalTouchForce = 1;  // TODO check value

}
