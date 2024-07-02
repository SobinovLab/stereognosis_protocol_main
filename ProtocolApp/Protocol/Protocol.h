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
#include "TeknicMotorApi.h"
#include "NIUsb6001card.h"
#include "CameraClient.h"
#include "TouchSensorClient.h"
#include "LedStrip.h"
#include "armCinterface.h"
#include "kinovaErrCode.h"


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
		bool isLedsOn();

		// General parameters TODO: load ip etc from ini
		ProtocolParameters params;

		//////// GUI interaction
		// Control of the protocol running
		std::atomic<bool> stopProtocol;
		std::atomic<bool> startTrial;
		std::atomic<bool> stopTrial;
		std::atomic<bool> deservesReward;     // user-GUI defined reward for the monkey. Overrides the calculated one.
		std::atomic<bool> loopAutomatically;  // Protocol automatically loops. Pressing Retreat deliberately stops the looping

		// main loop that is run in a thread when StartProtocol is clicked
		virtual void run();

        // motor
		TeknicMotorApi* motorHub = nullptr;

        // arm (replacing motorHub)
        KinovaArmClient* armClient = nullptr;
        bool armHomed = false;

		// current state of the protocol/trial
		void setCurrentState(ProtocolState state);
		ProtocolState getCurrentState();
		void trialStateGuiUpdate();

		void autoLoopToggle(const bool enable);

		// sets of gui variables
		CEdit* m_trialStatus;
		CButton* loopChk;
		void set_photoresistor_monitors(CStaticColor* front, CStaticColor* rear, CStaticColor* left, CStaticColor* right);
		void set_pressure_sensors_gui_controls(CEdit* serverLogCtrl);
		void set_trial_buttons(CButton* startTrialBtn, CButton* retreatBtn, CButton* retreatFlushBtn);

		//////// local devices
		// reward
		void reward();
		void reward(long duration);

		// light sensors
		std::atomic<bool> use_front_light_sensor = false;
		std::atomic<bool> use_rear_light_sensor = false;
        std::atomic<bool> use_left_arm_touch = false;
        std::atomic<bool> use_right_arm_touch = false;
        std::atomic<bool> reward_on_return = false;
        std::atomic<bool> monitor_passive_arm = false;
		int which_active_arm = 0; //negative for left, positive for right, 0 for unused
        int which_passive_arm = 0; //negative for left, positive for right, 0 for unused

		// motors
		bool were_motors_homed();
		int home_motors();
		void stop_motors();
		int motors_neutral_position();

		//////// connected devices
		// cameras
		void connect_camera_client_i(int i);
		void set_camera_i_gui_controls(int i, CEdit* serverLogCtrl);
		void disconnect_camera_client_i(int i);

		void send_config_to_cameras();
		int capture_single_frame();

		static const int NUM_CAMERAS = 4;

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
		std::vector<int> forceTargetIds;
		std::deque<double> forceTargets;
		void matchLoadedSessionTrialToParams(const std::vector<std::string>& line1, const std::vector<std::string>& line2, const std::vector<double>& vec);
		std::ofstream trialLogCsv;
		void openCsvLog();
		void addLineToCsvLog(const bool got_reward, const bool repeating,
			const long long trial_start_time, const long long log_sent_config_to_cameras, const long long object_in_position_time,
			const long long arm_liftoff_time,
			const long long log_started_camera_recording, const long long log_started_ps_recording,
			const long long log_started_ephys_recording, const long long log_sent_start_sync_messages, 
			const long long log_started_monitoring_ps,
			const long long trial_end_time,
			const long long log_starting_finishing_recordings, const long long log_sent_end_sync_messages,
			const long long log_stopped_camera_recordings, const long long log_stopped_ps_recordings,
			const long long trial_finished_time,
            const long long arm_return_time);
		void closeCsvLog();

		//////// local devices
		void initDevices();
		void releaseDevices();

		// photoresistor, reward
		NIUsb6001card m_NIUsb6001card;

		// photoresistors
		CStaticColor* m_frontPhotoresistorCtrl = nullptr;
		CStaticColor* m_rearPhotoresistorCtrl = nullptr;
		// used to monitor cover/uncover in seq with other calls
		std::chrono::steady_clock::time_point* _arm_at_rest_start_time = nullptr;
		// use in seq for waiting for start of the next trial
		// put _arm_at_rest_start_time to null before first call using going_to_monitor_arm_at_rest
		bool is_arm_at_rest();
		// call before the first call to is_arm_at_rest
		void prepare_to_monitor_arm_at_rest();
		// DEPRECATED and is not used
		int wait_until_arm_at_rest();
		// DEPRECATED and is not used
		int wait_until_arm_liftoff();

        //Watching arm lift off in a thread
        void armMonitoringThread();
        std::atomic<bool> allowInterupt;
        std::atomic<bool> monitoringThreadAlive = false;

		// ephys
		void start_ephys_recording();
		void break_ephys_recording();

		// LEDs
		LedStrip* ledStrip = nullptr;

		//////// connected devices
		void sync_message_trial_start();
		void sync_message_trial_end();
		
		// cameras
		CameraClient m_cameraClients[NUM_CAMERAS];
		
		//CameraClient m_cameraClient1;
		//CameraClient m_cameraClient2;
		// Additional cam servers
		//CameraClient m_cameraClient3;
		//CameraClient m_cameraClient4;
		

		void prepare_camera_recording();  // TODO future - cameras prepare capture
		int start_camera_recording();
		int start_camera_recording(long trial_number);
		void break_camera_recording();
		bool did_cameras_finish_saving();
		int wait_for_cameras_finish_saving();
		void send_camera_timestamp(int timestamp);

		// pressure sensors
		TouchSensorClient m_touchSensorClient;
		void start_pressure_sensor_recording();
		void start_pressure_sensor_recording(long trial_number);
		void send_pressure_sensor_timestamp(int timestamp);
		int break_pressure_sensor_recording();
		void wait_until_monkey_release();
		std::atomic<bool> stopWatch;
		void watch_early_grab();

		// smart sounds
		void playStartTaskTone();

		//////// running protocol support
		// calculated reward from monkey performance
		std::atomic<bool> m_earnedReward;
		std::atomic<bool> m_stopAsyncTrialConditionMonitor;
		std::vector<long long> m_force_target_start_times;
		std::vector<long long> m_started_touching_times;
		std::thread* m_asyncTrialSuccessMonitorThread = nullptr;
		void m_asyncTrialConditionMonitor();  // Monitors asynchronous conditions for trial end - force sensor press

};
