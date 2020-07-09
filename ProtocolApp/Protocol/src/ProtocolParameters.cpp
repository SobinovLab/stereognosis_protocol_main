#include "ProtocolParameters.h"

ProtocolParameters::ProtocolParameters()
{
	init();
}

ProtocolParameters::ProtocolParameters(const ProtocolParameters & protocolParams)
{
	this->rewardDuration = protocolParams.rewardDuration;
	this->nTrialsDesired = protocolParams.nTrialsDesired;
	this->acceleration = protocolParams.acceleration;
	this->speed = protocolParams.speed;
	this->position = protocolParams.position;
	this->sensorGraspingTime = protocolParams.sensorGraspingTime;
	this->intertrialTime = protocolParams.intertrialTime;
	this->maxWaitTime = protocolParams.maxWaitTime;
}

ProtocolParameters::~ProtocolParameters()
{
}

void ProtocolParameters::init()
{
	maxWaitTime = 5000;
	rewardDuration = 1000;
	nTrialsDesired = 150;
	acceleration = 2; // proportional level 1-10 (1 - 4000 RPM/S)
	speed = 2;        // proportional level 1-10 (1 - 700 RPM)
	position = 120;     // 1 to 240 mm -> proportional cycles ((-1) to (-105000) CNTs)
	sensorGraspingTime = 500; // msec
	intertrialTime = 1000; //msec
}