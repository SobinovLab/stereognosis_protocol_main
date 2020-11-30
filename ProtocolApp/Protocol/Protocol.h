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
#include "utilities\Sounds.h"
#include "utilities\Times.h"
#include "ProtocolParameters.h"
#include "TeknicMotorDevice.h"
#include "NIUsb6001card.h"

using namespace std;
using namespace chrono;

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

		// Control of the protocol running
		atomic<bool> stopProtocol;
		atomic<bool> startTrial;
		atomic<bool> stopTrial;
		atomic<bool> retreatedMotors;

		ProtocolState getCurrentState();

		void reward();
		void reward(long duration);

		// main loop that is run in a thread when StartProtocol is clicked
		virtual void run(CEdit* currentTrialGUICtrl);

		std::atomic<long> currentTrialNumber;

		// GUI stuff
		void set_photoresistor_monitors(CStaticColor* front, CStaticColor* rear);

	private:
		atomic<ProtocolState> protocolState;

		//////// GUI
		void updateCurrentTrialOnTheGUI(const long & nTotTrialsPlayedUntilNow, CEdit * currentTrialGUICtrl);

		// logging
		void logGoodTrial(const long& nCurrentTrial, const long& timeElapsedFromStartTaskToneToLiftsMonkeyArm, const long& timeElapsedFromStartTaskToneToPlatesTouch);
		void logBadTrial(const long& nCurrentTrial);

		//////// devices
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

		//////// running protocol support
		bool isTimeout(time_point<std::chrono::steady_clock>& startToneTime);
		void storeStartTime(time_point<std::chrono::steady_clock>& time);
		bool Protocol::isElapsedTheMinUncoveredTime(time_point<std::chrono::steady_clock>& photoresistorsUncoveredTime);


};
