/*********************************************************************
*
* Description:
*    This class manages the 3D trackpad device
*
*********************************************************************/
#pragma once

#include <windows.h>
#include <atomic>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <chrono>
#include <string>
#include "hmi_api.h"
#include "Logger.h"

#pragma comment(lib, "mchp_hmi.lib")

class Touchpad3DData
{
public:
	int running;
	int auto_calib;
	int last_gesture;
	int touch_detect;
	int air_wheel;
	int last_mouse;
	int mouse_countdown;

	std::chrono::steady_clock::time_point local_start_time;
	std::string global_sync_logger_time;
	long local_sync_dfs;  // duration from start

	/* 3D */
	hmi3d_position_t *out_pos;
	hmi3d_gesture_t *out_gesture;
	hmi3d_signal_t *out_sd;
	hmi3d_calib_t *out_calib;
	hmi3d_touch_t *out_touch;
	hmi3d_air_wheel_t *out_air_wheel;

	/* 2D */
	hmi2d_finger_pos_list_t *fingers;
	hmi2d_mouse_t *mouse;
	hmi2d_block_t *raw;

	/* output to file */
	std::string to_file_2d;
};

class Touchpad3DDevice 
{
	public:
		bool isTouching;
		bool freezeDataAcquired;
		Touchpad3DData m_data; // Sensor DATA
		Touchpad3DDevice();
		~Touchpad3DDevice();
		virtual void run();
		virtual void stop();

		virtual void syncWithGlobal(std::string & gst);
		virtual long getElapsedMilliSecsSinceStart();

	private:
		bool synced;
		bool m_stop;
		hmi_t * m_device;
		/* Global handles to screen-buffers */
		//	CConsoleLoggerEx m_PressureConsole;  // Pressure Canvas 
		/* Sensor Communincation */
		void init_device();
		void free_device();
		void init_data();
		void update_device();
};