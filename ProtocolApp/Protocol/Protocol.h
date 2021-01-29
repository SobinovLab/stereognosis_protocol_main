/*********************************************************************
*
* Description:
*    This class manages a Protocol:
*        by generating a sequence of trials
*        by managing the data from/to GUI
*        by managing the electronic devices involved in the protocol
*
*********************************************************************/
#pragma once

#include <windows.h>
#include <chrono>
#include <atomic>
#include <fstream>

#include "Logger.h"
#include "Sounds.h"
#include "Times.h"
#include "CsvParser.h"
#include "ProtocolParameters.h"
#include "TeknicMotorDevice.h"
#include "NIUsb6001card.h"
#include "CameraClient.h"
#include "TouchSensorClient.h"


enum class ProtocolState
{
	shutdown,
	initializing,
	trialReady,
	trialInProgress,
	trialFinalizing,
	shuttingDown,
};

class Protocol
{
	public:
		// is needed to trigger parameter refreshers
		CWnd* mainWindow;

		// creator-destructor
		Protocol();
		virtual ~Protocol();

		// What initialized correctly
		bool isMotorsOn();
		bool isRewardOn();
		bool isLightSensorsOn();
		bool isEphysOn();

		// General parameters TODO: load ip etc from ini
		ProtocolParameters params;

		//////// GUI interaction
		// Control of the protocol running
		std::atomic<bool> stopProtocol;
		std::atomic<bool> startTrial;
		std::atomic<bool> stopTrial;
		std::atomic<bool> deservesReward;  // user-GUI defined reward for the monkey. Overrides the calculated one.
		std::atomic<bool> loopAutomatically;  // Protocol automatically loops. Pressing Retreat deliberately stops the looping

		// main loop that is run in a thread when StartProtocol is clicked
		virtual void run();

		// current state of the protocol/trial
		void setCurrentState(ProtocolState state);
		ProtocolState getCurrentState();
		void trialStateGuiUpdate();

		void autoLoopToggle(const bool enable);

		// sets of gui variables
		CEdit* m_trialStatus;
		CButton* loopChk;
		void set_photoresistor_monitors(CStaticColor* front, CStaticColor* rear);
		void set_camera1_gui_controls(CEdit* serverLogCtrl);
		void set_camera2_gui_controls(CEdit* serverLogCtrl);
		void set_pressure_sensors_gui_controls(CEdit* serverLogCtrl);
		void set_trial_buttons(CButton* startTrialBtn, CButton* retreatBtn, CButton* retreatFlushBtn);

		//////// local devices
		// reward
		void reward();
		void reward(long duration);

		// motors
		bool were_motors_homed();
		void home_motors();

		//////// connected devices
		// cameras
		void connect_camera_client1();
		void connect_camera_client2();

		void disconnect_camera_client1();
		void disconnect_camera_client2();

		void send_config_to_cameras();
		int capture_single_frame();

		// pressure sensors
		void connect_pressure_sensors();
		void disconnect_pressure_sensors();

	private:
		// what the protocol is doing - trial state
		std::atomic<ProtocolState> protocolState;

		//////// GUI
		void push_variables_to_gui();
		void pull_variables_from_gui();

		CButton* startTrialBtn;
		CButton* retreatBtn;
		CButton* retreatFlushBtn;
		void trialFieldsToggle(bool enable);
		void trialFieldsEnableStart(bool enable);
		void trialFieldsEnableRetreat(bool enable);

		// logging and session params
		void matchLoadedSessionTrialToParams(const std::vector<std::string>& line1, const std::vector<std::string>& line2, const std::vector<double>& vec);
		std::ofstream trialLogCsv;
		void openCsvLog();
		void addLineToCsvLog(const bool got_reward, const bool repeating, const long long trial_start_time, const long long object_in_position_time,
			const long long trial_end_time, const long long trial_finished_time);
		void closeCsvLog();

		//////// local devices 
		void initDevices();
		void releaseDevices();

		// photoresistor, reward
		NIUsb6001card m_NIUsb6001card;

		// photoresistors
		CStaticColor* m_frontPhotoresistorCtrl = nullptr;
		CStaticColor* m_rearPhotoresistorCtrl = nullptr;

		// ephys
		void start_ephys_recording();
		void break_ephys_recording();

		// motor
		MotorAPI* motorHub = nullptr;

		//////// connected devices
		// cameras
		CameraClient m_cameraClient1;
		CameraClient m_cameraClient2;
		void prepare_camera_recording();  // TODO future - cameras prepare capture
		int start_camera_recording();
		int start_camera_recording(long trial_number);
		void break_camera_recording();
		bool did_cameras_finish_saving();
		int wait_for_cameras_finish_saving();

		// pressure sensors
		TouchSensorClient m_touchSensorClient;

		void start_pressure_sensor_recording();
		void start_pressure_sensor_recording(long trial_number);
		int break_pressure_sensor_recording();
		void wait_until_monkey_release();
		std::atomic<bool> stopWatch;
		void watch_early_grab();

		//////// running protocol support
		// calculated reward from monkey performance
		std::atomic<bool> m_earnedReward;
		std::atomic<bool> m_stopAsyncTrialConditionMonitor;
		std::thread* m_asyncTrialSuccessMonitorThread = nullptr;
		void m_asyncTrialConditionMonitor();  // Monitors asynchronous conditions for trial end - force sensor press

};
