#include "Protocol.h"

constexpr auto DATA_FOLDER = "./data";
constexpr auto TRIAL_NUM_STR = "Trial n.";
constexpr auto TRIAL_ABORT_STR = " Aborted";
constexpr auto FONT_TYPE = "Courier New";
constexpr auto PRECISION = "%03d";

constexpr auto FREQUENCY_START_TASK_TONE = 500;
constexpr auto FREQUENCY_ERROR_TONE = 250;
constexpr auto DURATION_TONE = 1000; //msecs
constexpr auto ABORT = 0;
constexpr auto MIN_UNCOVERED_TIME = 200; //msecs
constexpr auto TOUCHPAD_START_THREAD_DELAY = 1000; //msecs

#define DATE_TIME_FORMAT "%Y_%m_%d_%H_%M_%S"

Protocol::Protocol()
{
}

Protocol::~Protocol()
{
}

void Protocol::run(atomic<bool> * stopProtocol, NIUsb6001card * m_NIUsb6001card, CEdit * currentTrialGUICtrl)
{
	CreateDirectory(DATA_FOLDER, NULL);
	long nTotTrialsPlayedUntilNow = 0, nGoodTrials = 0;
	setFontGuiTrialsCounter(currentTrialGUICtrl);

	Touchpad3DDevice rightTouchPad, leftTouchPad;
	std::thread rightTouchPadThread(&Touchpad3DDevice::run, &rightTouchPad);
	Sleep(TOUCHPAD_START_THREAD_DELAY);
    rightTouchPad.syncWithGlobal(Logger::currentDateTimeInMilliseconds());
	std::thread	leftTouchPadThread(&Touchpad3DDevice::run, &leftTouchPad);
    leftTouchPad.syncWithGlobal(Logger::currentDateTimeInMilliseconds());

	TeknicMotorDevice motorHub;
	motorHub.init();
	while (!stopProtocol->load() && nTotTrialsPlayedUntilNow <= params.nTrialsDesired - 1)
	{
		updateCurrentTrialOnTheGUI(nTotTrialsPlayedUntilNow, nGoodTrials, currentTrialGUICtrl);
		bool isGoodTrial = false;
		/*
		* monkey arm moves before the �start tone� -> photoresistors are not covered -> trial aborted
		* -> �negative tone" (different from the initial tone)
		*/
		if (!isMotorMovementAborted(stopProtocol, m_NIUsb6001card, motorHub))
		{
			if (!stopProtocol->load()) playStartTaskTone();
			auto toneStartTime = chrono::steady_clock::now(), touchingPlatesTime = chrono::steady_clock::now(), photoresistorsUncoveredTime = chrono::steady_clock::now();
			bool TIME_OUT = false;
			/*
			* monkey arm is still on the arm-rest from the start task tone?
			* monkey arm doesn�t move for a max wait period after �start tone� -> TIMEOUT ->  trial aborted -> play �negative tone�
			* to avoid noise or a fake movements -> the arm is considered raised if the photoresistors are not covered for MIN_UNCOVERED_TIME
			*/
			while (!stopProtocol->load() && !TIME_OUT)
			{
				if (IS_REAR_PHOTORESISTOR_COVERED && IS_FRONT_PHOTORESISTOR_COVERED)	break;  // are the photoresistors still covered?
				TIME_OUT = isTimeout(toneStartTime);
			}
			while (!stopProtocol->load() && !TIME_OUT )
			{
				if (!IS_REAR_PHOTORESISTOR_COVERED && !IS_FRONT_PHOTORESISTOR_COVERED)
				{
					storeStartTime(photoresistorsUncoveredTime);
					break;
				}
				TIME_OUT = isTimeout(toneStartTime);
			}
			if (!TIME_OUT)
			{
				bool bothTouchSensorsAreToucheFirstTime = false;
				while (!stopProtocol->load() && !TIME_OUT)
				{
					if (areBothSensorsTouched(rightTouchPad, leftTouchPad))
					{
						if (!bothTouchSensorsAreToucheFirstTime)
						{
							storeStartTime(touchingPlatesTime);
							bothTouchSensorsAreToucheFirstTime = true;
						}
						if (getElapsedMilliSecsSince(touchingPlatesTime) >= params.sensorGraspingTime)
						{
							isGoodTrial = true; ++nGoodTrials;
							long microsecsFromStartTaskToneToLiftingMonkeyArm = getElapsedMicroSecsBetween(toneStartTime, photoresistorsUncoveredTime);
							long microsecsFromMonkeyArmRaisedToPlatesTouching = getElapsedMicroSecsBetween(photoresistorsUncoveredTime, touchingPlatesTime) - params.sensorGraspingTime;
							long totalMicrosecCurrentTrial = microsecsFromStartTaskToneToLiftingMonkeyArm + microsecsFromMonkeyArmRaisedToPlatesTouching;
							long rewardProportionalDuration = proportionalRewardCalculation(microToMillisecs(totalMicrosecCurrentTrial));
							startReward(m_NIUsb6001card, rewardProportionalDuration);
							logGoodTrial(nTotTrialsPlayedUntilNow + 1, microsecsFromStartTaskToneToLiftingMonkeyArm, microsecsFromMonkeyArmRaisedToPlatesTouching);
							break;
						}
					}
					TIME_OUT = isTimeout(toneStartTime);
				}
			}
		}
		if (!stopProtocol->load() && !isGoodTrial)
		{
			playErrorTone();
			logBadTrial(nTotTrialsPlayedUntilNow + 1);
		}
		++nTotTrialsPlayedUntilNow;
		Sleep(params.intertrialTime);
	}
	rightTouchPad.stop();
	leftTouchPad.stop();

	rightTouchPadThread.join();
    string filename = "./data/TouchPadRight_" + Logger::currentDateTime(DATE_TIME_FORMAT) + ".txt";
    saveTouchPadInfoToFile(string("right"), filename, rightTouchPad.m_data.to_file_2d);

	leftTouchPadThread.join();
    filename = "./data/TouchPadLeft_" + Logger::currentDateTime(DATE_TIME_FORMAT) + ".txt";
    saveTouchPadInfoToFile(string("left"), filename, leftTouchPad.m_data.to_file_2d);
}

void Protocol::saveTouchPadInfoToFile(string & rightleft, string & filename, string & m_data_string)
{
    if (std::FILE* f = std::fopen(filename.c_str(), "w")) {
        std::fprintf(f, "%s %s", rightleft.c_str(), m_data_string.c_str());
        std::fclose(f);

        string msg = " TouchPad " + rightleft + " saved to file";
        logInfo(msg.c_str());
    }
    else {
        string msg = " TouchPad " + rightleft + " could NOT save to file";
        logInfo(msg.c_str());
    }
}

bool Protocol::isElapsedTheMinUncoveredTime(time_point<std::chrono::steady_clock>& photoresistorsUncoveredTime)
{
	if (getElapsedMilliSecsSince(photoresistorsUncoveredTime) > MIN_UNCOVERED_TIME) return true;
	return false;
}

void Protocol::setFontGuiTrialsCounter(CEdit * currentTrialGUICtrl)
{
	CFont* cEditControlFont = new CFont();
	cEditControlFont->CreateFont(30, 0, 0, 0, FW_HEAVY, true, false, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T(FONT_TYPE));
	currentTrialGUICtrl->SetFont(cEditControlFont);
}

void Protocol::logBadTrial(const long& nCurrentTrial)
{
	string msg = TRIAL_NUM_STR + to_string(nCurrentTrial);
	msg = msg + TRIAL_ABORT_STR;
	logError(msg.c_str());
}

void Protocol::logGoodTrial(const long& nCurrentTrial, const long& microsecsFromStartTaskToneToLiftingMonkeyArm, const long& microsecsFromMonkeyArmRaisedToPlatesTouching)
{
	string msg = TRIAL_NUM_STR + to_string(nCurrentTrial);
	msg = msg + " monkey raised its arm in [microsecs]: " + to_string(microsecsFromStartTaskToneToLiftingMonkeyArm) + " -> monkey touched the plates in [microsecs]: " + to_string(microsecsFromMonkeyArmRaisedToPlatesTouching);
	logInfo(msg.c_str());
}

bool Protocol::isMotorMovementAborted(atomic<bool> * stopProtocol, NIUsb6001card* m_NIUsb6001card, TeknicMotorDevice& motorHub)
{
	motorHub.reset();
	if (!stopProtocol->load()) motorHub.home();
	// on waiting for the monkey puts the arm on the armrest before to start the trial
	while (!stopProtocol->load() && ( !IS_REAR_PHOTORESISTOR_COVERED || !IS_FRONT_PHOTORESISTOR_COVERED)) {}
	if (stopProtocol->load()) return true;
	// return true -> go() aborted
	return motorHub.go(&params.position, &params.speed, &params.acceleration);
}

void Protocol::updateCurrentTrialOnTheGUI(const long & nTotTrialsPlayedUntilNow, const long & nGoodTrials, CEdit * currentTrialGUICtrl)
{
	CStringA nTrialsConverted;
	nTrialsConverted.Format(_T(PRECISION), nTotTrialsPlayedUntilNow);
	CStringA nGoodTrialsConverted;
	nGoodTrialsConverted.Format(_T(PRECISION), nGoodTrials);
	currentTrialGUICtrl->SetWindowText(nGoodTrialsConverted + "/" + nTrialsConverted);
}

void Protocol::startReward(NIUsb6001card * m_NIUsb6001card, long & proportionalDuration)
{
		m_NIUsb6001card->reward(proportionalDuration);
}

void Protocol::playStartTaskTone()
{
	Beep(FREQUENCY_START_TASK_TONE, DURATION_TONE);
}

void Protocol::playErrorTone()
{
	Beep(FREQUENCY_ERROR_TONE, DURATION_TONE);
}

bool Protocol::isTimeout(time_point<std::chrono::steady_clock>& startToneTime)
{
	long timeElapsedFromStartTaskTone = getElapsedMicroSecsBetween(startToneTime, chrono::steady_clock::now());
	if (timeElapsedFromStartTaskTone > milliToMicrosecs(params.maxWaitTime)) return true;
	return false;
}

long Protocol::proportionalRewardCalculation(long long elapsed)
{
	double proportionalFactor = 1 - (double)elapsed / params.maxWaitTime;
	return (long)abs(params.rewardDuration * proportionalFactor);
}

bool Protocol::areBothSensorsTouched(Touchpad3DDevice& rightTouchPad, Touchpad3DDevice& leftTouchPad)
{
	if (rightTouchPad.isTouching && leftTouchPad.isTouching) return true;
	return false;
}

void Protocol::storeStartTime(time_point<std::chrono::steady_clock>& time)
{
	time = chrono::steady_clock::now();
}

long Protocol::microToMillisecs(const long & microsecs)
{
	return (long) (microsecs / 1000);
}

long Protocol::milliToMicrosecs(const long & millisecs)
{
	return millisecs * 1000;
}

long Protocol::getElapsedMilliSecsSince(time_point<std::chrono::steady_clock> & startTime)
{
	auto elapsed = duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - startTime);
	return (long)elapsed.count();
}

long Protocol::getElapsedMicroSecsBetween(time_point<std::chrono::steady_clock> & startTime, time_point<std::chrono::steady_clock> & endTime)
{
	auto elapsed = duration_cast<chrono::microseconds>(endTime - startTime);
	return (long)elapsed.count();
}
