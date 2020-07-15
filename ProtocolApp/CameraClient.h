#pragma once

#include <atomic>

class CameraClient
{
public:
	CameraClient();
	virtual ~CameraClient();

	void run();

	CString server_ip;
	long port;

	std::atomic<bool> disconnect;

	CEdit* clientStatusGuiEdt;
	CEdit* clientLogGuiEdt;

private:
	void setClientStatusGui(CString status);
	void appendClientLog(CString text);
};

