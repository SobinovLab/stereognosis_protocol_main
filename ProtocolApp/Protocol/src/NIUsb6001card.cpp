#include "NIUsb6001card.h"

using namespace std;

#pragma comment(lib, "NIDAQmx.lib")

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void* callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void* callbackData);

enum PhotoResistor { FRONT = 0, REAR };

void resetPhotoresistorGuiMonitor(CStaticColor* gui_monitor);
void updatePhotoresistorGuiMonitor(CStaticColor* gui_monitor, PhotoResistor photoResistor);
void stopTask(TaskHandle& taskHandle);
void stopAllTasks();
void logErrMsg(const int32& error);

const char* USB6001_CFG_FILE = "./configuration/config.ini";
constexpr const char* AI_TASK_NAME = "AI_task";
constexpr const char* PHOTORESISTORS_STATUS_TASK_NAME = "Photoresistors_Status_task";
constexpr const char* REWARD_SYSTEM_TASK_NAME = "Reward_System_task";
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
uInt8 PHOTORESISTORS_STATUS[2] = { 0, 0 }; // monitor buffer
int32 N_PHOTORESISTORS = 2;
static TaskHandle  AItaskHandle = 0, PhotoResistorStatus_taskHandle = 0, RewardSystem_taskHandle = 0;

atomic<bool> IS_REAR_PHOTORESISTOR_COVERED;
atomic<bool> IS_FRONT_PHOTORESISTOR_COVERED;

CStaticColor * FRONT_PHOTORESISTOR_GUI_MONITOR;
CStaticColor * REAR_PHOTORESISTOR_GUI_MONITOR;
COLORREF red = RGB(255, 0, 0);
COLORREF green = RGB(0, 255, 0);
DWORD sysColor = GetSysColor(COLOR_BTNFACE);
COLORREF normal = RGB(GetRValue(sysColor), GetGValue(sysColor), GetBValue(sysColor));

NIUsb6001card::NIUsb6001card()
{
}

NIUsb6001card::~NIUsb6001card()
{
	stopAllTasks();
}

void NIUsb6001card::config()
{
	int32   error = 0, AUTOSTART = 1;
	float64 TIMEOUT = 10.0;
	// m_physicalChanAI is used only for the continuous reading of the photoresistors status
	string m_physicalChanAI, m_physicalChanRewardSystem, m_physicalChanPhotoresistors;
	TCHAR value[32];
	GetPrivateProfileString(TEXT("RewardSystem"), TEXT("physicalChannel"), TEXT("Error loading physical acquisition channel"), value, 32, TEXT(USB6001_CFG_FILE));
	m_physicalChanRewardSystem = string(value);
	GetPrivateProfileString(TEXT("Photoresistors"), TEXT("physicalChannelDI"), TEXT("Error loading physical stimulation DI channel"), value, 32, TEXT(USB6001_CFG_FILE));
	m_physicalChanPhotoresistors = string(value);
	GetPrivateProfileString(TEXT("Photoresistors"), TEXT("physicalChannelAI"), TEXT("Error loading physical stimulation AI channel"), value, 32, TEXT(USB6001_CFG_FILE));
	m_physicalChanAI = string(value);

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/

	DAQmxErrChk(DAQmxCreateTask(AI_TASK_NAME, &AItaskHandle));
	DAQmxErrChk(DAQmxCreateAIVoltageChan(AItaskHandle, m_physicalChanAI.c_str(), "", DAQmx_Val_Cfg_Default, MIN_VOLTAGE, MAX_VOLTAGE, DAQmx_Val_Volts, NULL));
	DAQmxErrChk(DAQmxCfgSampClkTiming(AItaskHandle, "", SAMPLE_RATE, DAQmx_Val_Rising, DAQmx_Val_ContSamps, N_SAMPLES));

	DAQmxErrChk(DAQmxCreateTask(PHOTORESISTORS_STATUS_TASK_NAME, &PhotoResistorStatus_taskHandle));
	DAQmxErrChk(DAQmxCreateDIChan(PhotoResistorStatus_taskHandle, m_physicalChanPhotoresistors.c_str(), "", DAQmx_Val_ChanForAllLines));

	DAQmxErrChk(DAQmxRegisterEveryNSamplesEvent(AItaskHandle, DAQmx_Val_Acquired_Into_Buffer, N_SAMPLES, 0, EveryNCallback, NULL));
	DAQmxErrChk(DAQmxRegisterDoneEvent(AItaskHandle, 0, DoneCallback, NULL));

	DAQmxErrChk(DAQmxCreateTask(REWARD_SYSTEM_TASK_NAME, &RewardSystem_taskHandle));
	DAQmxErrChk(DAQmxCreateDOChan(RewardSystem_taskHandle, m_physicalChanRewardSystem.c_str(), "", DAQmx_Val_ChanForAllLines));

	deactivateReward();

Error:
	logErrMsg(error);
}

void NIUsb6001card::activateReward()
{
	int32   error = 0, AUTOSTART = 1;
	float64 TIMEOUT = 10.0;

	DAQmxErrChk(DAQmxWriteDigitalLines(RewardSystem_taskHandle, N_SAMPLES, AUTOSTART, TIMEOUT, DAQmx_Val_GroupByChannel, ACTIVATE_REWARD_BITS_MAP, NULL, NULL));

Error:
	logErrMsg(error);
}

void NIUsb6001card::deactivateReward()
{
	int32   error = 0, AUTOSTART = 1;
	float64 TIMEOUT = 10.0;

	DAQmxErrChk(DAQmxWriteDigitalLines(RewardSystem_taskHandle, N_SAMPLES, AUTOSTART, TIMEOUT, DAQmx_Val_GroupByChannel, DEACTIVATE_REWARD_BITS_MAP, NULL, NULL)); // reset the status

Error:
	logErrMsg(error);
}

void NIUsb6001card::start()
{
	int32 error = 0;
	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk(DAQmxStartTask(PhotoResistorStatus_taskHandle));
	DAQmxErrChk(DAQmxStartTask(AItaskHandle));

Error:
	logErrMsg(error);
}

void NIUsb6001card::stop()
{
	stopAllTasks();
	resetPhotoresistorsGuiMonitor();
}

void NIUsb6001card::resetPhotoresistorsGuiMonitor()
{
	resetPhotoresistorGuiMonitor(FRONT_PHOTORESISTOR_GUI_MONITOR);
	resetPhotoresistorGuiMonitor(REAR_PHOTORESISTOR_GUI_MONITOR);
}

void NIUsb6001card::reward(long & millisecs)
{
	activateReward();
	Sleep(millisecs);
	deactivateReward();
}

void NIUsb6001card::setFrontPhotoresistorMonitor(CStaticColor* gui_monitor)
{
	FRONT_PHOTORESISTOR_GUI_MONITOR = gui_monitor;
}

void NIUsb6001card::setRearPhotoresistorMonitor(CStaticColor* gui_monitor)
{
	REAR_PHOTORESISTOR_GUI_MONITOR = gui_monitor;
}

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
	int32   readDI, error = 0, nBytesBufferSize = 2 * N_SAMPLES;
	float64 TIMEOUT = 10.0;

	/*********************************************/
	// DAQmx Read Code
	/*********************************************/
	DAQmxErrChk(DAQmxReadDigitalLines(PhotoResistorStatus_taskHandle, N_SAMPLES, TIMEOUT, DAQmx_Val_GroupByChannel, PHOTORESISTORS_STATUS, N_PHOTORESISTORS, &readDI, &nBytesBufferSize, NULL));

	updatePhotoresistorGuiMonitor(REAR_PHOTORESISTOR_GUI_MONITOR, REAR);
	updatePhotoresistorGuiMonitor(FRONT_PHOTORESISTOR_GUI_MONITOR, FRONT);

	IS_REAR_PHOTORESISTOR_COVERED.store(PHOTORESISTORS_STATUS[REAR] == 0);
	IS_FRONT_PHOTORESISTOR_COVERED.store(PHOTORESISTORS_STATUS[FRONT] == 0);

Error:
	logErrMsg(error);
	return 0;
}

void updatePhotoresistorGuiMonitor(CStaticColor* gui_monitor, PhotoResistor photoResistor)
{
	if (gui_monitor != NULL) gui_monitor->SetBkColor(PHOTORESISTORS_STATUS[photoResistor] ? red : green);
}

void resetPhotoresistorGuiMonitor(CStaticColor* gui_monitor)
{
	if (gui_monitor != NULL) gui_monitor->SetBkColor(normal);
}

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	int32   error = 0;

	// Check to see if an error stopped the task.
	DAQmxErrChk(status);

Error:
	logErrMsg(error);
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
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		logError(errBuff);
		stopAllTasks();
	}
}

void stopAllTasks()
{
	stopTask(AItaskHandle);
	stopTask(PhotoResistorStatus_taskHandle);
	stopTask(RewardSystem_taskHandle);
}