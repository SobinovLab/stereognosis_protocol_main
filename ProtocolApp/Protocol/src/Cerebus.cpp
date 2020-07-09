#include <iostream>
#include "NIDAQmxConfigParser.h"
#include "NIDAQmxNoValidConfigurationEx.h"
#include "Logger.h"
#include "Cerebus.h"

using namespace nidaqmx;

#define CEREBUS_CONFIG_FOLDER string("./configuration/")
#define CEREBUS_CONTROLLER_CFG_FILE string("cerebus-interface.txt")
const int nRows = 12;
const int nCols = 12;
const uInt8 DIGITAL_PULSES_MAP[nCols][nRows] = {
	{1,0,0,0,0,0,0,0,0,0},
	{0,1,0,0,0,0,0,0,0,0},
	{0,0,1,0,0,0,0,0,0,0},
	{0,0,0,1,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0,0,0},
	{0,0,0,0,0,1,0,0,0,0},
	{0,0,0,0,0,0,1,0,0,0},
	{0,0,0,0,0,0,0,1,0,0},
	{0,0,0,0,0,0,0,0,1,0},
	{0,0,0,0,0,0,0,0,0,1}
};

#define DAQmxErrChk(functionCall, errMsg) checkErrors(error=(functionCall), errMsg)
constexpr auto TIMEOUT = 5.0;

void checkErrors(int code, const char * errorText) {
	if (DAQmxFailed(code)) {
		char errBuff[2048] = { '\0' };
		int dimBuff = 2048;
		DAQmxGetExtendedErrorInfo(errBuff, dimBuff);
		string customErrorMsg = string("Custom Error Msg: ") + string(errorText);
		logError(customErrorMsg.c_str());
		string daqErrorMsg = string("DAQmxErrorMsg: ") + string(errBuff);
		logError(daqErrorMsg.c_str());
	}
}

Cerebus::Cerebus()
{
	/*********************************************/
	// DAQmx Config Card Manager
	/*********************************************/
		NIDAQmxConfigParser  config(CEREBUS_CONFIG_FOLDER + CEREBUS_CONTROLLER_CFG_FILE);
		double m_minVoltage = config.getMinVoltage();		// double
		double m_maxVoltage = config.getMaxVoltage();		// double
		long m_sampleRate = config.getSampleRate();		// long
		long m_nSamples = config.getSamplesPerChannel();	// long
		long m_digitalLinesNum = config.getDigitalLinesNumber();// long
		long m_channelsNumber = config.getChannelsNumber();    // long
		string m_physicalChannels = config.getChannels();			// string
		string m_taskName = config.getTaskName();          // string
		string m_channelName = config.getChannelName();       // string
		if (m_digitalLinesNum <= 0) return;
		int error;
		DAQmxErrChk(DAQmxCreateTask(m_taskName.c_str(), &m_taskHandle), "DAQmxCreateTask - cfgAsDigitalWriter");
		DAQmxErrChk(DAQmxCreateDOChan(m_taskHandle, m_physicalChannels.c_str(), m_channelName.c_str(), DAQmx_Val_ChanForAllLines), "DAQmxCreateDOChan - cfgAsDigitalWriter");
		DAQmxErrChk(DAQmxStartTask(m_taskHandle), "DAQmxCreateDOChan - startTask");
}

Cerebus::~Cerebus()
{
}

void Cerebus::beginProtocol()
{
	send(Command::PROTOCOL_BEGIN);
}

void Cerebus::endProtocol()
{
	send(Command::PROTOCOL_END);
}

void Cerebus::beginTrial()
{
	send(Command::TRIAL_BEGIN);
}

void Cerebus::endTrial()
{
	send(Command::TRIAL_END);
}

void Cerebus::beginStim()
{
	send(Command::STIM_BEGIN);
}

void Cerebus::endStim()
{
	send(Command::STIM_END);
}

void Cerebus::beginPostTrial()
{
	send(Command::POST_TRIAL_BEGIN);
}

void Cerebus::endPostTrial()
{
	send(Command::POST_TRIAL_END);
}

void Cerebus::beginRecording()
{
	send(Command::START_RECORDING);
}

void Cerebus::endRecording()
{
	send(Command::STOP_RECORDING);
}

void Cerebus::send(Command cmd) {
	int NUM_SAMPLES_PER_CHANNEL = 1, error, AUTOSTART = 1; //true
	int row = (int)cmd;
	DAQmxErrChk(DAQmxWriteDigitalLines(m_taskHandle, NUM_SAMPLES_PER_CHANNEL, AUTOSTART, TIMEOUT, DAQmx_Val_GroupByChannel, &DIGITAL_PULSES_MAP[row][0], NULL, NULL), "DAQmxWriteDigitalLines - write digital pulse");
}