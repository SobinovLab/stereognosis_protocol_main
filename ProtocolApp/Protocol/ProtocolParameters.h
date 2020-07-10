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
		long maxWaitTime; //msecs
		long rewardDuration; //msecs
		long acceleration; // proportional level 1-10 (1 - 4000 RPM/S)
		long speed;        // proportional level 1-10 (1 - 700 RPM)
		long position;     // in mm -> proportional CNT -> cycles ((-1) to (-105000) CNTs)

		bool tstEnMotors;
		bool tstEnReward;
		bool tstEnCameras;
		bool tstEnLightSensors;
		bool tstEnTouchSensors;
		virtual void init();

		virtual bool isNiCardBeingUsed();
};