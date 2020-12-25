/*********************************************************************
*
* Description:
*    This class manages the input parameters for the protocol
*    The input parameters are mapped in the GUI
*
*********************************************************************/
#pragma once

using namespace std;

class ProtocolParameters
{
	public:
		ProtocolParameters();
		ProtocolParameters(const ProtocolParameters & protocolParams);
		virtual ~ProtocolParameters();
		/**
		*   Protocol Parameters
		*/
		double maxWaitTime;		//secs
		double intertrialWaitTime; // secs
		long rewardDuration;	//msecs
		long acceleration;		// proportional level 1-10 (1 - 4000 RPM/S)
		long speed;				// proportional level 1-10 (1 - 700 RPM)
		long position;			// in mm -> proportional CNT -> cycles ((-1) to (-105000) CNTs)

		// camera servers
		CString cs_ip1;
		long cs_port1;
		CString cs_ip2;
		long cs_port2;

		// camera config
		double cs_framerate;
		int cs_recordingPeriod;
		int cs_refSerial;
		CString cs_gain;
		CString cs_exposure;

		CString tss_ip;
		long tss_port;

		// testing and debugging flags
		// will try to enable motors on program launch
		bool tstEnMotors;
		// will try to enable reward on program launch
		bool tstEnReward;
		// will try to enable light sensors on program launch
		bool tstEnLightSensors;
		// will try to enable ephys sync signals on program launch
		bool tstEnEphys;

		virtual void init();

		virtual bool isNiCardBeingUsed();
};