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
#include "ProtocolParameters.h"
#include "TeknicMotorDevice.h"
#include "NIUsb6001card.h"

using namespace std;
using namespace chrono;

class Protocol
{
	public:
		ProtocolParameters params;
		Protocol();
		virtual ~Protocol();
		virtual void run( atomic<bool>* stopProtocol, atomic<bool>* startTrial, atomic<bool>* stopTrial, NIUsb6001card* m_NIUsb6001card, CEdit* currentTrialGUICtrl);

	private:
		void updateCurrentTrialOnTheGUI(const long & nTotTrialsPlayedUntilNow, CEdit * currentTrialGUICtrl);
		void startReward(NIUsb6001card * m_NIUsb6001card, long & proportionalDuration);
		long getElapsedMicroSecsBetween(time_point<std::chrono::steady_clock> & startTime, time_point<std::chrono::steady_clock> & endTime);
		long getElapsedMilliSecsSince(time_point<std::chrono::steady_clock> & startTime);
		long proportionalRewardCalculation(long long elapsed);
		//bool areBothSensorsTouched(Touchpad3DDevice & rightTouchPad, Touchpad3DDevice & leftTouchPad);
		void saveTouchPadInfoToFile(string& rightleft, string& filename, string& m_data_string);

		bool isMotorMovementAborted(atomic<bool> * stopProtocol, NIUsb6001card* m_NIUsb6001card, TeknicMotorDevice& motorHub);
		bool startForwardMovement(atomic<bool>* stopProtocol, atomic<bool>* stopTrial, NIUsb6001card* m_NIUsb6001card, TeknicMotorDevice& motorHub);
		
		void logGoodTrial(const long& nCurrentTrial, const long& timeElapsedFromStartTaskToneToLiftsMonkeyArm, const long& timeElapsedFromStartTaskToneToPlatesTouch);
		void logBadTrial(const long& nCurrentTrial);

		bool isTimeout(time_point<std::chrono::steady_clock>& startToneTime);
		void storeStartTime(time_point<std::chrono::steady_clock>& time);
		long microToMillisecs(const long & microsecs);
		long milliToMicrosecs(const long & millisecs);
		long secToMicrosecs(const double& millisecs);
		bool Protocol::isElapsedTheMinUncoveredTime(time_point<std::chrono::steady_clock>& photoresistorsUncoveredTime);

		void setFontGuiTrialsCounter(CEdit * currentTrialGUICtrl);
};