#include "NIUsb6001card.h"

// This class was originally written using GOTO statements, which is a horrid practice

using namespace std;

#pragma comment(lib, "NIDAQmx.lib")

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void* callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void* callbackData);

enum PhotoResistor { FRONT = 0, REAR, LEFTTOUCH, RIGHTTOUCH };

// TODO because of tons of global variables, these are here. Should be rewritten in the future
void stopTask(TaskHandle& taskHandle);
void stopAllTasks();
void resetPhotoresistorGuiMonitor(CStaticColor* gui_monitor);
void updatePhotoresistorGuiMonitor(CStaticColor* gui_monitor, PhotoResistor photoResistor);
void logErrMsg(const int32& error);

const char* USB6001_CFG_FILE = "./configuration/config.ini";
constexpr const char* AI_TASK_NAME = "AI_task";
constexpr const char* PHOTORESISTORS_STATUS_TASK_NAME = "Photoresistors_Status_task";
constexpr const char* REWARD_SYSTEM_TASK_NAME = "Reward_System_task";
constexpr const char* EPHYS_SYNC_TASK_NAME = "TTL_Sync_Ephys";
constexpr float64 NO_RESULT = 0;
constexpr float64 TIMEOUT = 60.0;
constexpr float64 MIN_VOLTAGE = -10.0;
constexpr float64 MAX_VOLTAGE = 10.0;

// this configuration permits to have a refresh of the photoresistors status 
// every 10 millisecs
float64 SAMPLE_RATE = 100.0; 
int32   N_SAMPLES = 1;

uInt8 ACTIVATE_REWARD_BITS_MAP[1] = { 1 };
uInt8 DEACTIVATE_REWARD_BITS_MAP[1] = { 0 };
uInt8 *PHOTORESISTORS_STATUS = (uInt8 *)calloc(4, sizeof(uInt8)); // monitor buffer 0 covered 1 uncovered
int32 N_PHOTORESISTORS = 4;
static TaskHandle  AItaskHandle = 0, PhotoResistorStatus_taskHandle = 0, RewardSystem_taskHandle = 0, ephysSync_taskHandle = 0; 

atomic<bool> IS_REAR_PHOTORESISTOR_COVERED;
atomic<bool> IS_FRONT_PHOTORESISTOR_COVERED;
atomic<bool> IS_LEFT_ARMSENSOR_TOUCHED;
atomic<bool> IS_RIGHT_ARMSENSOR_TOUCHED;

CStaticColor * FRONT_PHOTORESISTOR_GUI_MONITOR;
CStaticColor * REAR_PHOTORESISTOR_GUI_MONITOR;
CStaticColor * LEFT_TOUCH_GUI_MONITOR;
CStaticColor * RIGHT_TOUCH_GUI_MONITOR;

COLORREF white = RGB(255, 255, 255);
COLORREF black = RGB(0, 0, 0);
DWORD sysColor = GetSysColor(COLOR_BTNFACE);
COLORREF normal = RGB(GetRValue(sysColor), GetGValue(sysColor), GetBValue(sysColor));

NIUsb6001card::NIUsb6001card() {
}

NIUsb6001card::~NIUsb6001card() { 
	stopAllTasks(); 
}

int NIUsb6001card::config() {
	initializedCorrectly = false;
	int32   errorNumber = 0, AUTOSTART = 1;
	float64 TIMEOUT = 10.0;
	// m_physicalChanAI is used only for the continuous reading of the photoresistors status
	string m_physicalChanAI, m_physicalChanRewardSystem, m_physicalChanPhotoresistors, m_physicalChanEphysSync;
    string m_leftArmChannel, m_rightArmChannel;
	TCHAR value[32];

	// read default values from the .ini file
	GetPrivateProfileString(
		TEXT("RewardSystem"),   
		TEXT("physicalChannel"),   
		TEXT("Error loading physical reward sys channel"), 
		value, 32, TEXT(USB6001_CFG_FILE));
	m_physicalChanRewardSystem = string(value);

	GetPrivateProfileString(
		TEXT("Photoresistors"), 
		TEXT("physicalChannelDI"), 
		TEXT("Error loading physical stimulation DI channel"), 
		value, 32, TEXT(USB6001_CFG_FILE));
	m_physicalChanPhotoresistors = string(value);

	GetPrivateProfileString(
		TEXT("Photoresistors"), 
		TEXT("physicalChannelAI"), 
		TEXT("Error loading physical stimulation AI channel"), 
		value, 32, TEXT(USB6001_CFG_FILE));
	m_physicalChanAI = string(value);

	GetPrivateProfileString(
		TEXT("ephysSync"),      
		TEXT("physicalChannel"),   
		TEXT("Error loading ephysSync channel"), 
		value, 32, TEXT(USB6001_CFG_FILE));
	m_physicalChanEphysSync = string(value);

    GetPrivateProfileString(
		TEXT("MakeyMakey"),      
		TEXT("physicalDigitalChannelLeft"),   
		TEXT("Error loading left MakeyMakey channel"), 
		value, 32, TEXT(USB6001_CFG_FILE));
	m_leftArmChannel = string(value);

    GetPrivateProfileString(
		TEXT("MakeyMakey"),      
		TEXT("physicalDigitalChannelRight"),   
		TEXT("Error loading right MakeyMakey channel"), 
		value, 32, TEXT(USB6001_CFG_FILE));
	m_rightArmChannel = string(value);


	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/

	// TODO better logic - partial launch?

	// Main DAQ task
	errorNumber = DAQmxCreateTask(AI_TASK_NAME, &AItaskHandle);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	errorNumber = DAQmxCreateAIVoltageChan(AItaskHandle, m_physicalChanAI.c_str(), "", DAQmx_Val_Cfg_Default, 
		MIN_VOLTAGE, MAX_VOLTAGE, DAQmx_Val_Volts, NULL);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	errorNumber = DAQmxCfgSampClkTiming(AItaskHandle, "", SAMPLE_RATE, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 
		N_SAMPLES);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	errorNumber = DAQmxRegisterEveryNSamplesEvent(AItaskHandle, DAQmx_Val_Acquired_Into_Buffer, N_SAMPLES, 0, 
		EveryNCallback, NULL);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	errorNumber = DAQmxRegisterDoneEvent(AItaskHandle, 0, DoneCallback, NULL);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}

	// photoresistors - light sensors
	errorNumber = DAQmxCreateTask(PHOTORESISTORS_STATUS_TASK_NAME, &PhotoResistorStatus_taskHandle);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	errorNumber = DAQmxCreateDIChan(PhotoResistorStatus_taskHandle, m_physicalChanPhotoresistors.c_str(), "Photoresistors", 
		DAQmx_Val_ChanForAllLines);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}

	/*
    errorNumber = DAQmxCreateDIChan(PhotoResistorStatus_taskHandle, m_leftArmChannel.c_str(), "LeftArmHandle", 
		DAQmx_Val_ChanForAllLines);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	
    errorNumber = DAQmxCreateDIChan(PhotoResistorStatus_taskHandle, m_rightArmChannel.c_str(), "RightArmHandle", 
		DAQmx_Val_ChanForAllLines);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	*/
	// reward
	errorNumber = DAQmxCreateTask(REWARD_SYSTEM_TASK_NAME, &RewardSystem_taskHandle);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	errorNumber = DAQmxCreateDOChan(RewardSystem_taskHandle, m_physicalChanRewardSystem.c_str(), "", DAQmx_Val_ChanForAllLines);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}

	deactivateReward();

	// ephys
	errorNumber = DAQmxCreateTask(EPHYS_SYNC_TASK_NAME, &ephysSync_taskHandle);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	errorNumber = DAQmxCreateDOChan(ephysSync_taskHandle, m_physicalChanEphysSync.c_str(), "", DAQmx_Val_ChanForAllLines);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}

	enableEphysSync();

	// start the tasks
	errorNumber = start();

	if (!DAQmxFailed(errorNumber))
		initializedCorrectly = true;

	return errorNumber;
}

int NIUsb6001card::ephysSyncStart() {
	if (!wasInitializedCorrectly())
		return -1;

	int32   errorNumber = 0, AUTOSTART = 1;
	float64 TIMEOUT = 10.0;

	errorNumber = DAQmxWriteDigitalLines(ephysSync_taskHandle, N_SAMPLES, AUTOSTART, TIMEOUT, DAQmx_Val_GroupByChannel, 
		ACTIVATE_REWARD_BITS_MAP, NULL, NULL);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
	}
	return errorNumber;
}

int NIUsb6001card::ephysSyncStop() {
	if (!wasInitializedCorrectly())
		return -1;

	int32   errorNumber = 0, AUTOSTART = 1;
	float64 TIMEOUT = 10.0;

	errorNumber = DAQmxWriteDigitalLines(ephysSync_taskHandle, N_SAMPLES, AUTOSTART, TIMEOUT, DAQmx_Val_GroupByChannel, 
		DEACTIVATE_REWARD_BITS_MAP, NULL, NULL);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
	}
	return errorNumber;
}

int NIUsb6001card::enableEphysSync() {
	// This needs to be run during config() so that the line has a known initial state
	int32   errorNumber = 0, AUTOSTART = 1;
	float64 TIMEOUT = 10.0;

	errorNumber = DAQmxWriteDigitalLines(ephysSync_taskHandle, N_SAMPLES, AUTOSTART, TIMEOUT, DAQmx_Val_GroupByChannel, 
		DEACTIVATE_REWARD_BITS_MAP, NULL, NULL);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
	}
	return errorNumber;
}

bool NIUsb6001card::wasInitializedCorrectly()
{
	return initializedCorrectly;
}

int NIUsb6001card::activateReward() {
	int32   errorNumber = 0, AUTOSTART = 1;
	float64 TIMEOUT = 10.0;

	errorNumber = DAQmxWriteDigitalLines(RewardSystem_taskHandle, N_SAMPLES, AUTOSTART, TIMEOUT, DAQmx_Val_GroupByChannel, 
		ACTIVATE_REWARD_BITS_MAP, NULL, NULL);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
	}
	return errorNumber;
}

int NIUsb6001card::deactivateReward()
{
	int32   errorNumber = 0, AUTOSTART = 1;
	float64 TIMEOUT = 10.0;

	errorNumber = DAQmxWriteDigitalLines(RewardSystem_taskHandle, N_SAMPLES, AUTOSTART, TIMEOUT, DAQmx_Val_GroupByChannel, 
		DEACTIVATE_REWARD_BITS_MAP, NULL, NULL); // reset the status
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
	}
	return errorNumber;
}

int NIUsb6001card::start()
{
	int32 errorNumber = 0;

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/

	errorNumber = DAQmxStartTask(PhotoResistorStatus_taskHandle);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}

	errorNumber = DAQmxStartTask(AItaskHandle);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}

	return 0;
}

int NIUsb6001card::stop()
{
	if (!wasInitializedCorrectly())
		return -1;

	stopAllTasks();
	resetPhotoresistorsGuiMonitor();
	return 0;
}

void NIUsb6001card::resetPhotoresistorsGuiMonitor()
{
	resetPhotoresistorGuiMonitor(FRONT_PHOTORESISTOR_GUI_MONITOR);
	resetPhotoresistorGuiMonitor(REAR_PHOTORESISTOR_GUI_MONITOR);
}

int NIUsb6001card::reward(long & millisecs)
{
	if (!wasInitializedCorrectly())
		return -1;

	activateReward();
	Sleep(millisecs);
	deactivateReward();
	
	return 0;
}

void NIUsb6001card::setFrontPhotoresistorMonitor(CStaticColor* gui_monitor)
{
	FRONT_PHOTORESISTOR_GUI_MONITOR = gui_monitor;
}

void NIUsb6001card::setRearPhotoresistorMonitor(CStaticColor* gui_monitor)
{
	REAR_PHOTORESISTOR_GUI_MONITOR = gui_monitor;
}

void NIUsb6001card::setArmTouchSensors(CStaticColor* left, CStaticColor* right)
{
    LEFT_TOUCH_GUI_MONITOR = left;
    RIGHT_TOUCH_GUI_MONITOR = right;
}

int NIUsb6001card::TTLTest() {
	// Add hoc function to send 5 TTL pulses
	auto freq = 5.0; // Hz
	auto period = 1.0 / freq; // Period of the signal
	auto half_period = period / 2.0; 

	for (int i = 0; i < 5; ++i) {
		// Start the pulse
		ephysSyncStart();

		// Keep the pulse high for half the period
		std::this_thread::sleep_for(std::chrono::duration<double>(half_period));

		// Stop the pulse
		ephysSyncStop();

		// Keep the pulse low for the other half of the period
		std::this_thread::sleep_for(std::chrono::duration<double>(half_period));
	}

	return 0;
}

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
	int32   readDI, errorNumber = 0, nBytesBufferSize = 4 * N_SAMPLES;
	float64 TIMEOUT = 10.0;

	/*********************************************/
	// DAQmx Read Code
	/*********************************************/
	errorNumber = DAQmxReadDigitalLines(PhotoResistorStatus_taskHandle, N_SAMPLES, TIMEOUT, DAQmx_Val_GroupByChannel, 
		PHOTORESISTORS_STATUS, N_PHOTORESISTORS, &readDI, &nBytesBufferSize, NULL);
	if (DAQmxFailed(errorNumber)) {
		logErrMsg(errorNumber);
		return errorNumber;
	}
	
	updatePhotoresistorGuiMonitor(REAR_PHOTORESISTOR_GUI_MONITOR, REAR);
	updatePhotoresistorGuiMonitor(FRONT_PHOTORESISTOR_GUI_MONITOR, FRONT);

    updatePhotoresistorGuiMonitor(LEFT_TOUCH_GUI_MONITOR, LEFTTOUCH);
	updatePhotoresistorGuiMonitor(RIGHT_TOUCH_GUI_MONITOR, RIGHTTOUCH);

	IS_REAR_PHOTORESISTOR_COVERED.store(PHOTORESISTORS_STATUS[REAR] == 0);
	IS_FRONT_PHOTORESISTOR_COVERED.store(PHOTORESISTORS_STATUS[FRONT] == 0);

    IS_LEFT_ARMSENSOR_TOUCHED.store(PHOTORESISTORS_STATUS[LEFTTOUCH] == 0);
    IS_RIGHT_ARMSENSOR_TOUCHED.store(PHOTORESISTORS_STATUS[RIGHTTOUCH] == 0);

	return 0;
}

void updatePhotoresistorGuiMonitor(CStaticColor* gui_monitor, PhotoResistor photoResistor)
{
	if (gui_monitor != NULL) 
		gui_monitor->SetBkColor(PHOTORESISTORS_STATUS[photoResistor] ? white : black);
}

void resetPhotoresistorGuiMonitor(CStaticColor* gui_monitor)
{
	if (gui_monitor != NULL) 
		gui_monitor->SetBkColor(normal);
}

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	// Check to see if an error stopped the task.
	if (DAQmxFailed(status)) {
		logErrMsg(status);
		return status;
	}
	
	return 0;
}

void stopTask(TaskHandle & taskHandle)
{
	if (taskHandle) {
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		taskHandle = 0;
	}
}

void logErrMsg(const int32 & error)
{
	char    errBuff[2048] = { '\0' };
	string stringbuf;
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		stringbuf = string("NIUsb6001card: ") + errBuff;
		logError(stringbuf.c_str());
		stopAllTasks();
	}
}

void stopAllTasks()
{
	stopTask(AItaskHandle);
	stopTask(PhotoResistorStatus_taskHandle);
	stopTask(RewardSystem_taskHandle);
	stopTask(ephysSync_taskHandle);
}