#pragma once

#include <atomic>
#include "ProtocolParameters.h"

class CameraServer
{
public:
	CameraServer();
	virtual ~CameraServer();

	virtual void run(std::atomic<bool>* stopCameraServer, CEdit* serverStatusGuiEdt, CEdit* serverLogGuiEdt);

	CString ip;
	long port;

//private:
	void setServerStatusGui(CEdit* serverStatusGuiEdt, CString status);
	void appendServerLog(CEdit* serverLogGuiEdt, CString text);
};

