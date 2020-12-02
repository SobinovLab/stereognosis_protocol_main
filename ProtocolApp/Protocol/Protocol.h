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
		atomic<bool> stopProtocol;
		atomic<bool> startTrial;
		atomic<bool> stopTrial;
		atomic<bool> deservesReward;

		// main loop that is run in a thread when StartProtocol is clicked
		virtual void run();

		std::atomic<long> currentTrialNumber;
		ProtocolState getCurrentState();

		// sets of gui variables
		void set_photoresistor_monitors(CStaticColor* front, CStaticColor* rear);
		void set_camera1_gui_controls(CEdit* serverStatusCtrl, CEdit* serverLogCtrl);
		void set_camera2_gui_controls(CEdit* serverStatusCtrl, CEdit* serverLogCtrl);
		void set_pressure_sensors_gui_controls(CEdit* serverLogCtrl);
		void set_current_trial_gui_control(CEdit* currentTrialGuiCtrl);

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
		void capture_single_frame();

		// pressure sensors
		void connect_pressure_sensors();
		void disconnect_pressure_sensors();

	private:
		atomic<ProtocolState> protocolState;

		//////// GUI
		CEdit* m_currentTrialGuiCtrl;
		void updateCurrentTrialOnTheGui();

		// logging TODO: update
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
		TeknicMotorDevice* motorHub = nullptr;
		bool isMotorMovementAborted(atomic<bool> * stopProtocol);
		bool startForwardMovement();

		// ephys
		void start_ephys_recording();
		void break_ephys_recording();

		//////// connected devices
		// cameras
		CameraClient m_cameraClient1;
		CameraClient m_cameraClient2;
		void prepare_camera_recording();
		void start_camera_recording();
		void start_camera_recording(long trial_number);
		void break_camera_recording();

		// pressure sensors
		TouchSensorClient m_touchSensorClient;

		void start_pressure_sensor_recording();
		void start_pressure_sensor_recording(long trial_number);
		int break_pressure_sensor_recording();

		//////// running protocol support
		atomic<bool> m_earnedReward;
		atomic<bool> m_stopAsyncTrialConditionMonitor;
		std::thread* m_asyncTrialSuccessMonitorThread;
		void m_asyncTrialConditionMonitor();

};
