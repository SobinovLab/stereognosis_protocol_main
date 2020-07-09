#include "TeknicMotorDevice.h"
#include "NIUsb6001card.h"

TeknicMotorDevice::TeknicMotorDevice() : myMgr(SysManager::Instance()), myPort(nullptr), theNode(nullptr)
{
}

TeknicMotorDevice::~TeknicMotorDevice()
{
	stop();
	if (myMgr != NULL)
	{
		// Close down the ports
		myMgr->PortsClose();
	}
}

void TeknicMotorDevice::init()
{
	size_t portCount = 0;
	vector<string> comHubPorts;
	try
	{
		SysManager::FindComHubPorts(comHubPorts);
		//logInfo(string_format("Found %d SC Hubs\n", comHubPorts.size()).c_str());
		for (; portCount < comHubPorts.size() && portCount < NET_CONTROLLER_MAX; portCount++) {
			myMgr->ComHubPort(portCount, comHubPorts[portCount].c_str()); 	//define the first SC Hub port (port 0) to be associated 
																			// with COM portnum (as seen in device manager)
		}
		if (portCount < 0) {
			logError("Unable to locate SC hub port\n");
			return;
		}
		myMgr->PortsOpen(portCount); //Open the port
		myPort = &myMgr->Ports(PORT_NUM);
		int motors = myPort->NodeCount();
		while (motors != NUMBERS_OF_MOTORS)
		{
			motors = myPort->NodeCount();
			logError("Wrong number of motors available\n");
			Sleep(100);
		}
		theNode = &myPort->Nodes(HORIZONTAL_MOTOR);
		theNode->EnableReq(false);  //Ensure Node is disabled before loading config file
		reset();
		theNode->EnableReq(true);	//Enable node 
	}
	catch (mnErr& theErr)
	{
		int msgboxID = MessageBox(
			NULL,
			theErr.ErrorMsg,
			"Teknic Motors Manager Error",
			MB_ICONWARNING | MB_OK
		);

	}
}

bool TeknicMotorDevice::go(const long * positionInMillimeters, const long * speedLevel, const long * accelerationLevel)
{
	bool isAborted = false;
	theNode->Port.BrakeControl.BrakeSetting(0, BRAKE_ALLOW_MOTION);
	double currentPosCtn = theNode->Motion.PosnMeasured.Value();
	theNode->AccUnit(INode::RPM_PER_SEC);		//Set the units for Acceleration to RPM/SEC
	theNode->VelUnit(INode::RPM);				            //Set the units for Velocity to RPM
	int32_t newPosCtn = (int32_t)(convertPositionToCNTs(*positionInMillimeters));
	int32_t speed = (int32_t)(convertSpeedLevelToRPM(*speedLevel));
	int32_t acceleration = (int32_t)(convertAccLevelToRPMperSecs(*accelerationLevel));
	if ((newPosCtn == CONVERSION_ERROR) || (speed == CONVERSION_ERROR) || (acceleration == CONVERSION_ERROR)) // we are working with negative Positions -> toward to the chair
	{
		logError("Position error");
		isAborted = true;
	}
	else
	{
		theNode->Motion.VelLimit = speed;				        //Set Velocity Limit (RPM)
		theNode->Motion.AccLimit = acceleration;			//Set Acceleration Limit (RPM/Sec)
		theNode->Motion.MovePosnStart(newPosCtn);	//Execute encoder count move 
		double timeout = myMgr->TimeStampMsec() + theNode->Motion.MovePosnDurationMsec(newPosCtn) + 100;	 //define a timeout in case the node is unable to enable
		while (!theNode->Motion.MoveIsDone()) {
			if (!IS_REAR_PHOTORESISTOR_COVERED || !IS_FRONT_PHOTORESISTOR_COVERED || myMgr->TimeStampMsec() > timeout) {
				stop();
				isAborted = true;
				if (myMgr->TimeStampMsec() > timeout) {
					logError("Error: Timed out waiting for move to complete\n");
				}
				break;
			}
		}
	}
	theNode->Port.BrakeControl.BrakeSetting(0, BRAKE_PREVENT_MOTION);
	return isAborted;
}

bool TeknicMotorDevice::isMoving()
{
	if (theNode != NULL && !theNode->Motion.MoveIsDone()) return true;
	return false;
}

void TeknicMotorDevice::stop()
{
	if (theNode != NULL)
	{
		theNode->Motion.NodeStop(STOP_TYPE_ESTOP_ABRUPT);
	}
}

void TeknicMotorDevice::reset()
{
	if (theNode != NULL)
	{			   
		theNode->Status.AlertsClear();				//Clear Alerts on node 
		theNode->Motion.NodeStopClear();			//Clear Nodestops on Node  					
	}
}

void TeknicMotorDevice::home()
{
	if (theNode->Motion.Homing.HomingValid())
	{
		theNode->Motion.Homing.Initiate();
		while (!theNode->Motion.Homing.WasHomed()) {
		}
	}
}

long TeknicMotorDevice::convertPositionToCNTs(long valueInMillimeter)
{
	if ((valueInMillimeter < MIN_POSITION) || (valueInMillimeter > MAX_POSITION)) return CONVERSION_ERROR;
	return valueInMillimeter * MAX_DISTANCE_CNTS / MAX_POSITION;
}

long TeknicMotorDevice::convertSpeedLevelToRPM(long level)
{
	if ((level < MIN_ACC_LEVEL) || (level > MAX_ACC_LEVEL)) return CONVERSION_ERROR;
	return level * MAX_ACC_LIM_RPM_PER_SEC / MAX_ACC_LEVEL;
}

long TeknicMotorDevice::convertAccLevelToRPMperSecs(long level)
{
	if ((level < MIN_SPEED_LEVEL) || (level > MAX_SPEED_LEVEL)) return CONVERSION_ERROR;
	return level * MAX_VEL_LIM_RPM / MAX_SPEED_LEVEL;
}
