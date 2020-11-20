/*********************************************************************
*
* Description:
*    This class manages a Protocol:
*        by generating a sequence of trials
*        by managing the data from/to GUI
*        by managing the electronic devices involved in the protocol
*
*********************************************************************/
//#pragma once

#include <windows.h>
#include <chrono>
#include <atomic>
#include "Logger.h"
#include "Sounds.h"
#include "Times.h"
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

		// main loop that is run in a thread when StartProtocol is clicked
		virtual void run(NIUsb6001card* m_NIUsb6001card, CEdit* currentTrialGUICtrl);

		std::atomic<long> currentTrialNumber;

	private:
		atomic<ProtocolState> protocolState;

		//////// GUI
		void updateCurrentTrialOnTheGUI(const long & nTotTrialsPlayedUntilNow, CEdit * currentTrialGUICtrl);
		void setFontGuiTrialsCounter(CEdit* currentTrialGUICtrl);

		// logging
		void logGoodTrial(const long& nCurrentTrial, const long& timeElapsedFromStartTaskToneToLiftsMonkeyArm, const long& timeElapsedFromStartTaskToneToPlatesTouch);
		void logBadTrial(const long& nCurrentTrial);

		//////// devices
		void initDevices();
		void releaseDevices();

		// reward
		void startReward(NIUsb6001card * m_NIUsb6001card, long & proportionalDuration);
		long proportionalRewardCalculation(long long elapsed);

		// motor
		NIUsb6001card m_NIUsb6001card;
		TeknicMotorDevice* motorHub = nullptr;
		bool isMotorMovementAborted(atomic<bool> * stopProtocol, NIUsb6001card* m_NIUsb6001card, TeknicMotorDevice* motorHub);
		bool startForwardMovement(NIUsb6001card* m_NIUsb6001card, TeknicMotorDevice* motorHub);

		//////// running protocol support
		bool isTimeout(time_point<std::chrono::steady_clock>& startToneTime);
		void storeStartTime(time_point<std::chrono::steady_clock>& time);
		bool Protocol::isElapsedTheMinUncoveredTime(time_point<std::chrono::steady_clock>& photoresistorsUncoveredTime);


};
