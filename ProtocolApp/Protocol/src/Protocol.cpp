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

void Protocol::run(atomic<bool>* stopProtocol, atomic<bool>* startTrial, atomic<bool>* stopTrial, atomic<bool>* retreatedMotors, 
	NIUsb6001card* m_NIUsb6001card, CEdit* currentTrialGUICtrl)
{
	// TODO?: Create directory for log storage
	CreateDirectory(DATA_FOLDER, NULL);
	setFontGuiTrialsCounter(currentTrialGUICtrl);

	// TODO: Load all trials from session config file

	// Initialize the motor
	TeknicMotorDevice motorHub;
	if (params.tstEnMotors) {
		motorHub.init();
	}

	// TODO class member variable?
	long nTotTrialsPlayedUntilNow = 0;
	// Run the protocol loop
	while (!stopProtocol->load())
	{
		updateCurrentTrialOnTheGUI(nTotTrialsPlayedUntilNow, currentTrialGUICtrl);
		// TODO: load the parameters of the next trial

		// TODO: conditions for waiting for the start of the next trial
		while (!startTrial->load() && !stopProtocol->load()) {}

		// TODO: start recordings

		// TODO: start motors
		startTrial->store(false);

		m_NIUsb6001card->ephysSyncStart();

		if (startForwardMovement(stopProtocol, stopTrial, m_NIUsb6001card, motorHub))
		{
			retreatedMotors->store(false);
			Sounds::playStartTaskTone();

			auto toneStartTime = chrono::steady_clock::now();

			// wait for stop trial signal NO TIMEOUT - timeout in the touch sensor monitor
			while (!stopProtocol->load() && !stopTrial->load()) {}
			//while (!stopProtocol->load() && !stopTrial->load() && !isTimeout(toneStartTime)) {}

			stopTrial->store(false);
			if (params.tstEnMotors) {
				motorHub.reset();
				motorHub.home();
			}
			retreatedMotors->store(true);
			m_NIUsb6001card->ephysSyncStop();
		}

		// TODO Conditions for success or fail at the trial 
		++nTotTrialsPlayedUntilNow;

		// TODO wait for the signal from recording devices that the data has been saved

		// TODO display the progress of saving on the GUI

	}
}

bool Protocol::isElapsedTheMinUncoveredTime(time_point<std::chrono::steady_clock>& photoresistorsUncoveredTime)
{
	if (Times::getElapsedMilliSecsSince(photoresistorsUncoveredTime) > MIN_UNCOVERED_TIME) return true;
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
	if (params.tstEnMotors)
		motorHub.reset();
	if (!stopProtocol->load() && params.tstEnMotors)
		motorHub.home();
	// on waiting for the monkey puts the arm on the armrest before to start the trial
	//while (!stopProtocol->load() && ( !IS_REAR_PHOTORESISTOR_COVERED || !IS_FRONT_PHOTORESISTOR_COVERED)) {}

	if (stopProtocol->load()) 
		return true;
	// return true -> go() aborted
	if (params.tstEnMotors)
		return motorHub.go(&params.position, &params.speed, &params.acceleration);
	else
		return true;
}

/// <summary>
/// 
/// </summary>
/// <param name="stopProtocol"></param>
/// <param name="stopTrial"></param>
/// <param name="m_NIUsb6001card"></param>
/// <param name="motorHub"></param>
/// <returns>True iff the motor movement started as planned or no motor initialized via testing</returns>
bool Protocol::startForwardMovement(atomic<bool>* stopProtocol, atomic<bool>* stopTrial, NIUsb6001card* m_NIUsb6001card, TeknicMotorDevice& motorHub)
{
	if (params.tstEnMotors) {
		motorHub.reset();
		motorHub.home();
	}

	if (stopProtocol->load() || stopTrial->load())
		return false;

	if (params.tstEnMotors)
		return !motorHub.go(&params.position, &params.speed, &params.acceleration);
	else
		return true;
}

void Protocol::updateCurrentTrialOnTheGUI(const long & nTotTrialsPlayedUntilNow, CEdit * currentTrialGUICtrl)
{
	CStringA nTrialsConverted;
	nTrialsConverted.Format(_T(PRECISION), nTotTrialsPlayedUntilNow);
	currentTrialGUICtrl->SetWindowText(nTrialsConverted);
	currentTrialNumber.store(nTotTrialsPlayedUntilNow);
}

void Protocol::startReward(NIUsb6001card * m_NIUsb6001card, long & proportionalDuration)
{
		m_NIUsb6001card->reward(proportionalDuration);
}

bool Protocol::isTimeout(time_point<std::chrono::steady_clock>& startToneTime)
{
	long timeElapsedFromStartTaskTone = Times::getElapsedMicroSecsBetween(startToneTime, chrono::steady_clock::now());
	if (timeElapsedFromStartTaskTone > Times::secToMicrosecs(params.maxWaitTime)) return true;
	return false;
}

long Protocol::proportionalRewardCalculation(long long elapsed)
{
	double proportionalFactor = 1 - (double)elapsed / params.maxWaitTime;
	return (long)abs(params.rewardDuration * proportionalFactor);
}

void Protocol::storeStartTime(time_point<std::chrono::steady_clock>& time)
{
	time = chrono::steady_clock::now();
}