#pragma once

#include "CStaticColor.h"
#include <vector>
#include <atomic>     
#include <chrono>   
#include <algorithm>  
#include <NIDAQmx.h>
#include "Logger.h"

extern atomic<bool> IS_REAR_PHOTORESISTOR_COVERED, IS_FRONT_PHOTORESISTOR_COVERED;

class NIUsb6001card
{
	public:
		  NIUsb6001card();
		  virtual ~NIUsb6001card();
		  void start(); 
		  void stop(); 
		  void config();
		  void reward(long & millisecs);  
		  void setFrontPhotoresistorMonitor(CStaticColor * gui_monitor);
		  void setRearPhotoresistorMonitor(CStaticColor* gui_monitor);
		  void resetPhotoresistorsGuiMonitor();
		  void activateReward();
		  void deactivateReward();
};