#include "Protocol.h"

constexpr auto DATA_FOLDER = "./data";
constexpr auto TRIAL_NUM_STR = "Trial n.";
constexpr auto TRIAL_ABORT_STR = " Aborted";
constexpr auto PRECISION = "%03d";

constexpr auto FREQUENCY_START_TASK_TONE = 500;
constexpr auto FREQUENCY_ERROR_TONE = 250;
constexpr auto DURATION_TONE = 1000; //msecs
constexpr auto ABORT = 0;
constexpr auto MIN_UNCOVERED_TIME = 200; //msecs
constexpr auto TOUCHPAD_START_THREAD_DELAY = 1000; //msecs

#define DATE_TIME_FORMAT "%Y_%m_%d_%H_%M_%S"

ProtocolState Protocol::getCurrentState()
{
	return protocolState.load();
}

Protocol::Protocol()
{
	this->stopProtocol.store(false);
	this->startTrial.store(false);
	this->stopTrial.store(false);
	this->retreatedMotors.store(false);

	this->protocolState.store(ProtocolState::shutdown);
}

Protocol::~Protocol()
{
	// if incorrect termination
	releaseDevices();
}

void Protocol::run(NIUsb6001card* m_NIUsb6001card, CEdit* currentTrialGUICtrl)
{
	this->protocolState.store(ProtocolState::initializing);
	// TODO?: Create directory for log storage
	CreateDirectory(DATA_FOLDER, NULL);

	// TODO: Load all trials from session config file

	// Initialize all devices
	initDevices();

	// TODO class member variable?
	long nTotTrialsPlayedUntilNow = 0;
	// Run the protocol loop
	while (!this->stopProtocol.load())
	{
		updateCurrentTrialOnTheGUI(nTotTrialsPlayedUntilNow, currentTrialGUICtrl);
		// TODO: load the parameters of the next trial

		this->protocolState.store(ProtocolState::trialReady);
		// TODO: conditions for waiting for the start of the next trial
		while (!this->startTrial.load() && !this->stopProtocol.load()) {}

		this->protocolState.store(ProtocolState::trialInProgress);
		// TODO: start recordings

		// TODO: start motors
		this->startTrial.store(false);

		m_NIUsb6001card->ephysSyncStart();

		if (startForwardMovement(m_NIUsb6001card, motorHub))
		{
			this->retreatedMotors.store(false);
			Sounds::playStartTaskTone();

			auto toneStartTime = chrono::steady_clock::now();

			// wait for stop trial signal NO TIMEOUT - timeout in the touch sensor monitor
			while (!this->stopProtocol.load() && !this->stopTrial.load()) {}
			//while (!stopProtocol->load() && !stopTrial->load() && !isTimeout(toneStartTime)) {}

			this->stopTrial.store(false);
			if (params.tstEnMotors) {
				motorHub->reset();
				motorHub->home();
			}
			this->retreatedMotors.store(true);
			m_NIUsb6001card->ephysSyncStop();
		}


		this->protocolState.store(ProtocolState::trialFinalizing);

		// TODO Conditions for success or fail at the trial
		++nTotTrialsPlayedUntilNow;

		// TODO wait for the signal from recording devices that the data has been saved

		// TODO display the progress of saving on the GUI

	}

	// release all devices
	releaseDevices();
}

bool Protocol::isElapsedTheMinUncoveredTime(time_point<std::chrono::steady_clock>& photoresistorsUncoveredTime)
{
	if (Times::getElapsedMilliSecsSince(photoresistorsUncoveredTime) > MIN_UNCOVERED_TIME) return true;
	return false;
}

void Protocol::logGoodTrial(const long& nCurrentTrial, const long& microsecsFromStartTaskToneToLiftingMonkeyArm, const long& microsecsFromMonkeyArmRaisedToPlatesTouching)
{
	string msg = TRIAL_NUM_STR + to_string(nCurrentTrial);
	msg = msg + " monkey raised its arm in [microsecs]: " + to_string(microsecsFromStartTaskToneToLiftingMonkeyArm) + " -> monkey touched the plates in [microsecs]: " + to_string(microsecsFromMonkeyArmRaisedToPlatesTouching);
	logInfo(msg.c_str());
}

void Protocol::logBadTrial(const long& nCurrentTrial)
{
	string msg = TRIAL_NUM_STR + to_string(nCurrentTrial);
	msg = msg + TRIAL_ABORT_STR;
	logError(msg.c_str());
}

void Protocol::initDevices()
{
	// motor
	if (params.tstEnMotors) {
		if (motorHub) {
			// WARNING: motor hub already initialized
		}
		else {
			motorHub = new TeknicMotorDevice();
			motorHub->init();
		}
	}
}

void Protocol::releaseDevices()
{
	// motor
	if (params.tstEnMotors) {
		if (motorHub) {  // check if nullptr
			delete motorHub;
			motorHub = nullptr;
		}
	}
}

bool Protocol::isMotorMovementAborted(atomic<bool> * stopProtocol, NIUsb6001card* m_NIUsb6001card, TeknicMotorDevice* motorHub)
{
	if (params.tstEnMotors)
		motorHub->reset();
	if (!stopProtocol->load() && params.tstEnMotors)
		motorHub->home();
	// on waiting for the monkey puts the arm on the armrest before to start the trial
	//while (!stopProtocol->load() && ( !IS_REAR_PHOTORESISTOR_COVERED || !IS_FRONT_PHOTORESISTOR_COVERED)) {}

	if (stopProtocol->load())
		return true;
	// return true -> go() aborted
	if (params.tstEnMotors)
		return motorHub->go(&params.position, &params.speed, &params.acceleration);
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
bool Protocol::startForwardMovement(NIUsb6001card* m_NIUsb6001card, TeknicMotorDevice* motorHub)
{
	if (params.tstEnMotors) {
		motorHub->reset();
		motorHub->home();
	}

	if (this->stopProtocol.load() || this->stopTrial.load())
		return false;

	if (params.tstEnMotors)
		return !motorHub->go(&params.position, &params.speed, &params.acceleration);
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
