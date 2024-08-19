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
#include <deque>

#include <nlohmann/json.hpp>

#include "Times.h"
#include "Directories.h"


class ProtocolParameters
{
private:
	// attempt to load this one before loading the local relative copy
	const std::string global_default_directory = "C:/PrehensionProtocol/";
	const std::string global_default_pp_filename = "main_protocol_parameters.json";
	
	// default json file that populates ProtocolParameters
	const std::string default_protocol_parameters_json = "./configuration/protocol_parameters.json";

	// where to look for a latest CSV with the description of the session
	std::string primary_session_file_directory = "C:/PrehensionProtocol/session_configs/";
	std::string secondary_session_file_directory = "./session_configs/";
	// default output directory for trial log
	std::string primary_session_log_directory = "C:/PrehensionProtocol/session_logs/";
	std::string secondary_session_log_directory = "./data/";


public:
	ProtocolParameters();
	virtual ~ProtocolParameters();

	virtual void init();

	void set_log_filename(const std::string& timestamp_str);

	bool identify_pc(std::string& pc);

	int load_json();
	int load_json(std::string filename);

	CString try_finding_session_csv();
	CString make_log_filename(const std::string& timestamp_str);

	/**
	*   Protocol Parameters
	*/

	// protocol
	double maxWaitTime = 20;		//secs
	double intertrialWaitTime = 2; // secs
	CString session_filename;  // try_finding_session_csv()
	CString session_log_filename;  // make_log_filename
	long rewardDuration = 1000;	//msecs
	bool disable_looping_on_manual_retreat = true;

	// photoresistors
	// how long needs to pass for the protocol to recognize the switch
	double photoresistor_status_switch_delay = 250;  // msec

	// trial
	int trial_number = 0;
	int counter = 0; // This always increments
	int total_trials = 0;
	double pos_translation_x = 0;
    double pos_translation_height = 0;
    double pos_translation_depth = 0;
	double pos_tilt = 0;
    double pos_pitch = 0;
    double pos_yaw = 0;
	double pos_aperture = 0;

	// camera servers
	std::vector<CString> cs_ips = { "205.208.87.188", "205.208.63.128" };
	long cs_port = 63874;

	/*
	CString cs_ip1 = "205.208.87.188";
	long cs_port1 = 63874;
	CString cs_ip2 = "205.208.63.128";
	long cs_port2 = 63874;
	*/

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
	int leds_early_target_force_lightup = 1;
	int leds_number = 40;

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

	// sounds
	bool sounds_modulation_enabled = false;
	double sounds_minforce = 1;
	double sounds_maxforce = 4;
	double sounds_minfreq = 450;
	double sounds_maxfreq = 550;

	// motors
	std::string motors_axes_filename = "./configuration/axes_stereognosis.json";
	std::string motors_motors_filename = "./configuration/motors_stereognosis1.json";
};