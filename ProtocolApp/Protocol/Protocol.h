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
		atomic<bool> retreatedMotors;

		// main loop that is run in a thread when StartProtocol is clicked
		virtual void run(CEdit* currentTrialGUICtrl);

		std::atomic<long> currentTrialNumber;
		ProtocolState getCurrentState();

		// sets of gui variables
		void set_photoresistor_monitors(CStaticColor* front, CStaticColor* rear);
		void set_camera1_gui_controls(CEdit* serverStatusCtrl, CEdit* serverLogCtrl);
		void set_camera2_gui_controls(CEdit* serverStatusCtrl, CEdit* serverLogCtrl);

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
		void prepare_camera_recording();
		void start_camera_recording();
		void start_camera_recording(long trial_number);
		void break_camera_recording();

	private:
		atomic<ProtocolState> protocolState;

		//////// GUI
		void updateCurrentTrialOnTheGUI(const long & nTotTrialsPlayedUntilNow, CEdit * currentTrialGUICtrl);

		// logging
		void logGoodTrial(const long& nCurrentTrial, const long& timeElapsedFromStartTaskToneToLiftsMonkeyArm, const long& timeElapsedFromStartTaskToneToPlatesTouch);
		void logBadTrial(const long& nCurrentTrial);

		//////// local devices
		void initDevices();
		void releaseDevices();

		// photoresistor, motor, reward
		NIUsb6001card m_NIUsb6001card;

		// photoresistors
		CStaticColor* m_frontPhotoresistorCtrl = nullptr;
		CStaticColor* m_rearPhotoresistorCtrl = nullptr;

		// reward
		long proportionalRewardCalculation(long long elapsed);

		// motor
		TeknicMotorDevice* motorHub = nullptr;
		bool isMotorMovementAborted(atomic<bool> * stopProtocol);
		bool startForwardMovement();

		//////// connected devices
		// cameras
		CameraClient m_cameraClient1;
		CameraClient m_cameraClient2;

		//////// running protocol support
		bool isTimeout(time_point<std::chrono::steady_clock>& startToneTime);
		void storeStartTime(time_point<std::chrono::steady_clock>& time);
		bool isElapsedTheMinUncoveredTime(time_point<std::chrono::steady_clock>& photoresistorsUncoveredTime);

};
