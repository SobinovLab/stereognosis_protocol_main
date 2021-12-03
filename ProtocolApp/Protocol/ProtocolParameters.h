/*********************************************************************
*
* Description:
*    This class manages the input parameters for the protocol
*    The input parameters are mapped in the GUI
*
*********************************************************************/
#pragma once
#include <string>

#include "Times.h"
#include "Folders.h"


class ProtocolParameters
{
private:
	// where to look for a latest CSV with the description of the session
	std::string default_session_file_directory = "./session_configs/";
	// default output directory for trial log
	std::string default_session_log_directory = "./data/";

	CString try_finding_session_csv();
	CString make_log_filename();

public:
	ProtocolParameters();
	virtual ~ProtocolParameters();

	virtual void init();

	/**
	*   Protocol Parameters
	*/

	// protocol
	double maxWaitTime;		//secs
	double intertrialWaitTime; // secs
	CString session_filename;
	CString session_log_filename;
	long rewardDuration;	//msecs

	// trial
	int trial_number;
	int total_trials;
	double pos_translation_x;			
	double pos_tilt;
	double pos_aperture;

	// camera servers
	CString cs_ip1;
	long cs_port1;
	CString cs_ip2;
	long cs_port2;

	// camera config
	double cs_framerate;
	int cs_recordingPeriod;
	int cs_refSerial;
	CString cs_gain;
	CString cs_exposure;
	int cs_capture_n_frames;

	// pressure sensor server
	CString tss_ip;
	long tss_port;

	// pressure sensor config
	double targetForce;
	double targetForceRelRangeMin;
	double targetForceRelRangeMax;
	double targetForceTotalMinThreshold;
	double thresholdPeriod;  // seconds
	double thresholdForceEachProportion;
	double minimalTouchForce;

};