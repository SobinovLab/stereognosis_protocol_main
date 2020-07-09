/*********************************************************************
*
* Description:
*    This class manages a Teknic motor 
* 
*********************************************************************/
#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include "Logger.h"
#include <stdlib.h>
#include "pubSysCls.h"	

#define CONVERSION_ERROR -1

#define MAX_ACC_LEVEL 10
#define MAX_SPEED_LEVEL 10
#define MAX_POSITION 240	 

#define MIN_ACC_LEVEL 1
#define MIN_SPEED_LEVEL 1
#define MIN_POSITION 0.1	 

#define MAX_ACC_LIM_RPM_PER_SEC	4000
#define MAX_VEL_LIM_RPM			700
#define MAX_DISTANCE_CNTS		-105000	// --->toward chair direction 
#define TIME_TILL_TIMEOUT		10000	//The timeout used for homing(ms)

#define HORIZONTAL_MOTOR   0
#define ROTATIONAL_MOTOR   1
#define RIGHT_DISH_MOTOR   2
#define LEFT_DISH_MOTOR    3

#define NUMBERS_OF_MOTORS   4
#define NUMBERS_OF_ATTEMPTS   10
#define PORT_NUM   0

using namespace std;
using namespace sFnd;

#pragma comment(lib, "sFoundation20.lib")

class TeknicMotorDevice 
{
	public:
		TeknicMotorDevice();
		~TeknicMotorDevice();
		void init();
		/* 
		 * Acceleration: proportional level 1-10 (1 - 4000 RPM/S)
		 * Speed:        proportional level 1-10 (1 - 700 RPM)
		 * Position:     1 to 240 mm -> proportional cycles ((-1) to (-105000) CNTs)
		 * Return a boolean value to notify if the action has been aborted (true) or not
		*/
		bool go(const long * positionInMillimeters, const long * speedLevel, const long * accelerationLevel);
		void stop();
		bool isMoving();
		void home();
		void reset();
	private:
		/*
		*   Create the SysManager object. This object will coordinate actions among various ports
		*   and within nodes. In this example we use this object to setup and open our port.
		*/
		SysManager * myMgr;	//Create System Manager myMgr
		IPort *myPort;
		INode * theNode;
		long convertPositionToCNTs(long valueInMillimeter);
		long convertSpeedLevelToRPM(long level);
		long convertAccLevelToRPMperSecs(long level);
};		