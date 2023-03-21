#include "ProtocolParameters.h"

using namespace std;
using json = nlohmann::json;

CString ProtocolParameters::try_finding_session_csv()
{
	string fname;

	string session_file_directory;
	if (Directories::path_exists(primary_session_file_directory))
		session_file_directory = primary_session_file_directory;
	else
		session_file_directory = secondary_session_file_directory;

	if (Directories::find_latest_csv(session_file_directory, fname)) {
		return CString(fname.c_str());
	}

	// solely for debugging
	if (Directories::find_latest_csv("./App/session_configs", fname)) {  // debugging
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

	string session_log_directory;
	if (Directories::path_exists(primary_session_log_directory))
		session_log_directory = primary_session_log_directory;
	else
		session_log_directory = secondary_session_log_directory;

	file_basename = session_log_directory + file_basename;

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
	if (load_json() < 0) {
		logWarning("Error during loading of default protocol parameters file. Using hardcoded values.");
	}

	session_filename = try_finding_session_csv();
	session_log_filename = make_log_filename();

}

bool ProtocolParameters::identify_pc(std::string& pc)
{
	if (!Directories::grep_one(global_default_directory, regex("PC=.*"), pc))
		return false;

	if (pc.size() > 3)
		pc = pc.substr(3);
	else
		pc = "";

	return true;
}

int ProtocolParameters::load_json()
{
	string gdppj, pc;
	if (identify_pc(pc))
		gdppj = global_default_directory + pc + "/" + global_default_pp_filename;
	else
		gdppj = global_default_directory + global_default_pp_filename;

	if (Directories::path_exists(gdppj))
		return load_json(gdppj);
	return load_json(default_protocol_parameters_json);
}

int ProtocolParameters::load_json(std::string filename)
{
	string buf;

	// load settings file
	ifstream ifs(filename);
	if (ifs.fail()) {
		buf = "Could not open protocol parameters JSON file: " + filename + ". Check if it exists.";
		logError(buf.c_str());
		return -1;
	}
	buf = "Loading protocol parameters JSON file: " + filename + ".";
	logInfo(buf.c_str());

	// parse it
	json pp_json;
	try
	{
		pp_json = json::parse(ifs);
	}
	catch (const json::parse_error& e)
	{
		buf = "Parse error during protocol parameters JSON file read: " + string(e.what());
		logError(buf.c_str());
		return -2;
	}

	// PROTOCOL
	try
	{
		json protocol_json = pp_json.at("protocol");

		maxWaitTime = protocol_json.value("maxWaitTime", maxWaitTime);
		intertrialWaitTime = protocol_json.value("intertrialWaitTime", intertrialWaitTime);
		primary_session_file_directory = protocol_json.value("primary_session_file_directory", primary_session_file_directory);
		secondary_session_file_directory = protocol_json.value("secondary_session_file_directory", secondary_session_file_directory);
		primary_session_log_directory = protocol_json.value("primary_session_log_directory", primary_session_log_directory);
		secondary_session_log_directory = protocol_json.value("secondary_session_log_directory", secondary_session_log_directory);
		rewardDuration = protocol_json.value("rewardDuration", rewardDuration);

		photoresistor_status_switch_delay = protocol_json.value("photoresistor_status_switch_delay", photoresistor_status_switch_delay);

	}
	catch (const json::exception& e)
	{
		buf = "Error reading 'protocol' entry of json file. Using defaults." + string(e.what());
        logWarning(buf.c_str());
	}

	// TRIAL
	try
	{
		json trial_json = pp_json.at("trial");

		pos_translation_x = trial_json.value("pos_translation_x", pos_translation_x);
		pos_tilt = trial_json.value("pos_tilt", pos_tilt);
		pos_aperture = trial_json.value("pos_aperture", pos_aperture);

	}
	catch (const json::exception& e)
	{
		buf = "Error reading 'trial' entry of json file. Using defaults." + string(e.what());
        logWarning(buf.c_str());
	}

	// CAMERAS
	try
	{
		json cameras_json = pp_json.at("cameras");

        cs_ip1 = ((string) cameras_json.value("ip1", cs_ip1)).c_str();
        cs_port1 = cameras_json.value("port1", cs_port1);
        cs_ip2 = ((string)cameras_json.value("ip2", cs_ip2)).c_str();
        cs_port2 = cameras_json.value("port2", cs_port2);
        cs_framerate = cameras_json.value("framerate", cs_framerate);
        cs_recordingPeriod = cameras_json.value("recordingPeriod", cs_recordingPeriod);
        cs_refSerial = cameras_json.value("refSerial", cs_refSerial);
        cs_exposure = ((string) cameras_json.value("exposure", cs_exposure)).c_str();
        cs_gain = ((string) cameras_json.value("gain", cs_gain)).c_str();
        cs_capture_n_frames = cameras_json.value("capture_n_frames", cs_capture_n_frames);

	}
	catch (const json::exception& e)
	{
		buf = "Error reading 'cameras' entry of json file. Using defaults." + string(e.what());
        logWarning(buf.c_str());
	}

    // PRESSURE_SENSOR
	//TODO add number of target forces
    try
    {
        json pressure_sensor_json = pp_json.at("pressure_sensor");

        tss_ip = ((string) pressure_sensor_json.value("ip", tss_ip)).c_str();
        tss_port = pressure_sensor_json.value("port", tss_port);
        targetForce = pressure_sensor_json.value("targetForce", targetForce);
        targetForceRelRangeMin = pressure_sensor_json.value("targetForceRelRangeMin", targetForceRelRangeMin);
        targetForceRelRangeMax = pressure_sensor_json.value("targetForceRelRangeMax", targetForceRelRangeMax);
        targetForceTotalMinThreshold = pressure_sensor_json.value("targetForceTotalMinThreshold", targetForceTotalMinThreshold);
        targetForceTotalMax = pressure_sensor_json.value("targetForceTotalMax", targetForceTotalMax);
        thresholdPeriod = pressure_sensor_json.value("thresholdPeriod", thresholdPeriod);
        thresholdForceEachProportion = pressure_sensor_json.value("thresholdForceEachProportion", thresholdForceEachProportion);
        minimalTouchForce = pressure_sensor_json.value("minimalTouchForce", minimalTouchForce);

    }
    catch (const json::exception& e)
    {
        buf = "Error reading 'pressure_sensor' entry of json file. Using defaults." + string(e.what());
        logWarning(buf.c_str());
    }

	// LEDS
	try
	{
		json leds_json = pp_json.at("leds");

		leds_com_port = leds_json.value("com_port", leds_com_port);
		leds_comPortFriendlyName = leds_json.value("comPortFriendlyName", leds_comPortFriendlyName);
		leds_run_test = leds_json.value("run_test", leds_run_test);
		leds_number = leds_json.value("number", leds_number);

		json top_color = leds_json.at("top_stripe_color");
		leds_top_stripe_color_red = top_color[0];
		leds_top_stripe_color_green = top_color[1];
		leds_top_stripe_color_blue = top_color[2];
		leds_top_stripe_brightness = leds_json.value("top_brightness", leds_top_stripe_brightness);
		leds_top_stripe_reverse_order = leds_json.value("top_reverse_order", leds_top_stripe_reverse_order);

		json bottom_color = leds_json.at("bottom_stripe_color");
		leds_bottom_stripe_color_red = bottom_color[0];
		leds_bottom_stripe_color_green = bottom_color[1];
		leds_bottom_stripe_color_blue = bottom_color[2];
		leds_bottom_stripe_brightness = leds_json.value("bottom_brightness", leds_bottom_stripe_brightness);
		leds_bottom_stripe_reverse_order = leds_json.value("bottom_reverse_order", leds_bottom_stripe_reverse_order);
		leds_early_target_force_lightup = leds_json.value("early_target_force_lightup", leds_early_target_force_lightup);
	}
	catch (const json::exception& e)
	{
		buf = "Error reading 'leds' entry of json file. Using defaults." + string(e.what());
		logWarning(buf.c_str());
	}

	// sounds
	try
	{
		json sounds_json = pp_json.at("sounds");

		int smed = sounds_json.value("enabled", (int) sounds_modulation_enabled);
		sounds_modulation_enabled = (bool)smed;

		sounds_minforce = sounds_json.value("minforce", sounds_minforce);
		sounds_maxforce = sounds_json.value("maxforce", sounds_maxforce);
		sounds_minfreq = sounds_json.value("minfreq", sounds_minfreq);
		sounds_maxfreq = sounds_json.value("maxfreq", sounds_maxfreq);
	}
	catch (const json::exception& e)
	{
		buf = "Error reading 'sounds' entry of json file. Using defaults." + string(e.what());
		logWarning(buf.c_str());
	}

	// motors
	try
	{
		json motors_json = pp_json.at("motors");

		motors_axes_filename = motors_json.value("axes_filename", motors_axes_filename);
		motors_motors_filename = motors_json.value("motors_filename", motors_motors_filename);
	}
	catch (const json::exception& e)
	{
		buf = "Error reading 'motors' entry of json file. Using defaults." + string(e.what());
		logWarning(buf.c_str());
	}

	return 0;
}
