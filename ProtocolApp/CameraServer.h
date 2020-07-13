#pragma once

#include <atomic>

class CameraServer
{
public:
	CameraServer();
	virtual ~CameraServer();

	virtual void start();
	virtual void stop();

	std::atomic<bool> startCameraRecording;
	std::atomic<bool> stopCameraRecording;

private:

};

