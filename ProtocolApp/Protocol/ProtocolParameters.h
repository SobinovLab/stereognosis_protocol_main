/*********************************************************************
*
* Description:
*    This class manages the input parameters for the protocol
*    The input parameters are mapped in the GUI
*
*********************************************************************/
#pragma once
#include <string>
#include <fstream>

#include <nlohmann/json.hpp>

#include "Times.h"
#include "Folders.h"


class ProtocolParameters
{
private:
	// where to look for a latest CSV with the description of the session
	std::string default_session_file_directory = "./session_configs/";
	// default output directory for trial log
	std::string default_session_log_directory = "./data/";
	// default json file that populates ProtocolParameters
	std::string default_protocol_parameters_json = "./configuration/protocol_parameters.json";


public:
	ProtocolParameters();
	virtual ~ProtocolParameters();

	virtual void init();

	int load_json();
	int load_json(std::string filename);

	CString try_finding_session_csv();
	CString make_log_filename();

	/**
	*   Protocol Parameters
	*/

	// protocol
	double maxWaitTime = 20;		//secs
	double intertrialWaitTime = 2; // secs
	CString session_filename;  // try_finding_session_csv()
	CString session_log_filename;  // make_log_filename
	long rewardDuration = 1000;	//msecs

	// trial
	int trial_number = 0;
	int total_trials = 0;
	double pos_translation_x = 115;			
	double pos_tilt = 0;
	double pos_aperture = 0;

	// camera servers
	CString cs_ip1 = "205.208.87.188";
	long cs_port1 = 63874;
	CString cs_ip2 = "205.208.63.128";
	long cs_port2 = 63874;

	// camera config
	double cs_framerate = 50;
	int cs_recordingPeriod = 60;
	int cs_refSerial = 19340298;
	CString cs_gain = "20";
	CString cs_exposure = "2500";
	int cs_capture_n_frames = 10;

	// pressure sensor server
	CString tss_ip = "205.208.87.188";
	long tss_port = 54940;

	// pressure sensor config
	double targetForce = 0.75;
	double targetForceRelRangeMin = -0.5;
	double targetForceRelRangeMax = 0.5;
	double targetForceTotalMinThreshold = 0.1;
	double targetForceTotalMax = 4;
	double thresholdPeriod = 1;  // seconds
	double thresholdForceEachProportion = 0.2;
	double minimalTouchForce = 0.5;

	// LEDs
	std::string leds_com_port = "\\\\.\\COM3";
	std::string leds_comPortFriendlyName = "Arduino Uno";
	int leds_run_test = 1;

	int leds_top_stripe_color_red = 244;
	int leds_top_stripe_color_green = 67;
	int leds_top_stripe_color_blue = 54;
	double leds_top_stripe_brightness = 0.5;
	int leds_top_stripe_reverse_order = 1;

	int leds_bottom_stripe_color_red = 76;
	int leds_bottom_stripe_color_green = 175;
	int leds_bottom_stripe_color_blue = 85;
	double leds_bottom_stripe_brightness = 0.5;
	int leds_bottom_stripe_reverse_order = 1;

};