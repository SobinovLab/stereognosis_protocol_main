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
#include "Logger.h"
#include "Sounds.h"
#include "Times.h"
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
		// creator-destructor
		Protocol();
		virtual ~Protocol();

		// General parameters TODO: load ip etc from ini
		ProtocolParameters params;

		//////// GUI interaction
		// Control of the protocol running
		std::atomic<bool> stopProtocol;
		std::atomic<bool> startTrial;
		std::atomic<bool> stopTrial;
		std::atomic<bool> deservesReward;  // user-GUI defined reward for the monkey. Overrides the calculated one.
		std::atomic<bool> loopAutomatically;  // TODO make gui interface to this switch - it is updated in the Protocol, too

		// main loop that is run in a thread when StartProtocol is clicked
		virtual void run();

		std::atomic<long> currentTrialNumber;
		// Not used meaningfully right now, good for understanding and maybe future
		ProtocolState getCurrentState();

		// sets of gui variables
		void set_photoresistor_monitors(CStaticColor* front, CStaticColor* rear);
		void set_camera1_gui_controls(CEdit* serverStatusCtrl, CEdit* serverLogCtrl);
		void set_camera2_gui_controls(CEdit* serverStatusCtrl, CEdit* serverLogCtrl);
		void set_pressure_sensors_gui_controls(CEdit* serverLogCtrl);
		void set_current_trial_gui_control(CEdit* currentTrialGuiCtrl);
		void set_trial_buttons(CButton* startTrialBtn, CButton* retreatBtn, CButton* retreatFlushBtn);

		//////// local devices
		void reward();
		void reward(long duration);

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
		// Not used meaningfully right now, good for understanding and maybe future
		std::atomic<ProtocolState> protocolState;

		//////// GUI
		CEdit* m_currentTrialGuiCtrl;
		void updateCurrentTrialOnTheGui();

		CButton* startTrialBtn;
		CButton* retreatBtn;
		CButton* retreatFlushBtn;
		void trialFieldsToggle(bool enable);
		void trialFieldsEnableStart(bool enable);
		void trialFieldsEnableRetreat(bool enable);

		// logging TODO: update/remove
		void logGoodTrial(const long& nCurrentTrial, const long& timeElapsedFromStartTaskToneToLiftsMonkeyArm, const long& timeElapsedFromStartTaskToneToPlatesTouch);
		void logBadTrial(const long& nCurrentTrial);

		//////// local devices  TODO: get information if the devices/card are connected from the card
		void initDevices();
		void releaseDevices();

		// photoresistor, motor, reward
		NIUsb6001card m_NIUsb6001card;

		// photoresistors
		CStaticColor* m_frontPhotoresistorCtrl = nullptr;
		CStaticColor* m_rearPhotoresistorCtrl = nullptr;

		// motor
		MotorAPI* motorHub = nullptr;
		bool isMotorMovementAborted();
		bool startForwardMovement();

		// ephys
		void start_ephys_recording();
		void break_ephys_recording();

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

		//////// running protocol support
		// calculated reward from monkey performance
		std::atomic<bool> m_earnedReward;
		std::atomic<bool> m_stopAsyncTrialConditionMonitor;
		std::thread* m_asyncTrialSuccessMonitorThread;
		void m_asyncTrialConditionMonitor();  // Monitors asynchronous conditions for trial end - force sensor press

};
