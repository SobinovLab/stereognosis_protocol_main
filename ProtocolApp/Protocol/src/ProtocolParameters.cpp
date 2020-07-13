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

	this->cs_ip = protocolParams.cs_ip;
	this->cs_port = protocolParams.cs_port;
	this->cs_framerate = protocolParams.cs_framerate;

	this->tstEnMotors = protocolParams.tstEnMotors;
	this->tstEnReward = protocolParams.tstEnReward;
	this->tstEnCameras = protocolParams.tstEnCameras;
	this->tstEnLightSensors = protocolParams.tstEnLightSensors;
	this->tstEnTouchSensors = protocolParams.tstEnTouchSensors;
}

ProtocolParameters::~ProtocolParameters()
{
}

void ProtocolParameters::init()
{
	maxWaitTime = 10;  // sec
	rewardDuration = 1000;
	acceleration = 2; // proportional level 1-10 (1 - 4000 RPM/S)
	speed = 2;        // proportional level 1-10 (1 - 700 RPM)
	position = 115;     // 1 to 240 mm -> proportional cycles ((-1) to (-105000) CNTs)

	cs_ip = "localhost";
	cs_port = 63874;
	cs_framerate = 100;

	tstEnMotors = false;
	tstEnReward = false;
	tstEnLightSensors = false;
	//tstEnMotors = true;
	//tstEnReward = true;
	//tstEnLightSensors = true;
	tstEnCameras = true;
	tstEnTouchSensors = false;
}

bool ProtocolParameters::isNiCardBeingUsed()
{
	return tstEnLightSensors || tstEnMotors || tstEnReward;
}
