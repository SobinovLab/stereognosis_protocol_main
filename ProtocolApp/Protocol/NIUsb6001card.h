#pragma once

#include "CStaticColor.h"
#include <vector>
#include <atomic>     
#include <chrono>   
#include <algorithm>  
#include <NIDAQmx.h>
#include "Logger.h"

extern std::atomic<bool> IS_REAR_PHOTORESISTOR_COVERED, IS_FRONT_PHOTORESISTOR_COVERED, IS_LEFT_ARMSENSOR_TOUCHED, IS_RIGHT_ARMSENSOR_TOUCHED;

class NIUsb6001card
{
private:
    bool initializedCorrectly = false;  // set by config()

    int start();  // starts the tasks

    // reward
    int activateReward();
    int deactivateReward();

    // ephys
    int enableEphysSync();
public:
    // init/deinit
    NIUsb6001card();
    virtual ~NIUsb6001card();
    int stop();
    int config();  // actually configures the card

    // reward
    int reward(long & millisecs);

    // light sensors
    void setFrontPhotoresistorMonitor(CStaticColor * gui_monitor);
    void setRearPhotoresistorMonitor(CStaticColor* gui_monitor);
    void setArmTouchSensors(CStaticColor*, CStaticColor*);
    int TTLTest();
    void resetPhotoresistorsGuiMonitor();

    // ephys
    int ephysSyncStart();
    int ephysSyncStop();

    bool wasInitializedCorrectly();
};
