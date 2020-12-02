#include "ProtocolParameters.h"

ProtocolParameters::ProtocolParameters()
{
	init();
}

ProtocolParameters::ProtocolParameters(const ProtocolParameters & protocolParams)
{
	this->rewardDuration = protocolParams.rewardDuration;
	this->acceleration = protocolParams.acceleration;
	this->speed = protocolParams.speed;
	this->position = protocolParams.position;
	this->maxWaitTime = protocolParams.maxWaitTime;
	this->intertrialWaitTime = protocolParams.intertrialWaitTime;

	this->cs_ip1 = protocolParams.cs_ip1;
	this->cs_port1 = protocolParams.cs_port1;
	this->cs_ip2 = protocolParams.cs_ip2;
	this->cs_port2 = protocolParams.cs_port2;
	this->cs_framerate = protocolParams.cs_framerate;
	this->cs_recordingPeriod = protocolParams.cs_recordingPeriod;
	this->cs_refSerial = protocolParams.cs_refSerial;
	this->cs_exposure = protocolParams.cs_exposure;
	this->cs_gain = protocolParams.cs_gain;

	this->tss_ip = protocolParams.tss_ip;
	this->tss_port = protocolParams.tss_port;

	this->tstEnMotors = protocolParams.tstEnMotors;
	this->tstEnReward = protocolParams.tstEnReward;
	this->tstEnLightSensors = protocolParams.tstEnLightSensors;
	this->tstEnEphys = protocolParams.tstEnEphys;
}

ProtocolParameters::~ProtocolParameters()
{
}

void ProtocolParameters::init()
{
	maxWaitTime = 20;  // sec
	intertrialWaitTime = 1; // sec

	rewardDuration = 1000;
	acceleration = 2; // proportional level 1-10 (1 - 4000 RPM/S)
	speed = 2;        // proportional level 1-10 (1 - 700 RPM)
	position = 115;     // 1 to 240 mm -> proportional cycles ((-1) to (-105000) CNTs)

	cs_ip1 = "205.208.87.188";
	cs_port1 = 63874;
	cs_ip2 = "205.208.63.128";
	cs_port2 = 63874;
	cs_framerate = 50;
	cs_recordingPeriod = 25;
	cs_refSerial = 19194009;
	cs_exposure = "2500";
	cs_gain = "20";

	tss_ip = "localhost";
	tss_port = 54940;

	//tstEnMotors = false;
	//tstEnReward = false;
	//tstEnLightSensors = false;
	//tstEnEphys = false;
	tstEnMotors = true;
	tstEnReward = true;
	tstEnLightSensors = true;
	tstEnEphys = true;
}

bool ProtocolParameters::isNiCardBeingUsed()
{
	return tstEnLightSensors || tstEnMotors || tstEnReward || tstEnEphys;
}
