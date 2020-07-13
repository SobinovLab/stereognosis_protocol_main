#include "stdafx.h"
#include "CameraServer.h"

CameraServer::CameraServer()
{
	startCameraRecording.store(false);
	stopCameraRecording.store(false);
}

CameraServer::~CameraServer()
{
}

void CameraServer::start()
{
}

void CameraServer::stop()
{
}
