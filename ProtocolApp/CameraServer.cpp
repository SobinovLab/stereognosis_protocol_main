#include "stdafx.h"
#include "CameraServer.h"

CameraServer::CameraServer()
{
	ip = "localhost";
	port = 60000;
}

CameraServer::~CameraServer()
{
}

void CameraServer::run(std::atomic<bool>* stopCameraServer, CEdit* serverStatusGuiEdt, CEdit* serverLogGuiEdt)
{
	setServerStatusGui(serverStatusGuiEdt, _T("Starting..."));
	appendServerLog(serverLogGuiEdt, TEXT("Starting server...\n"));


	CStringA buf;
	buf.Format(TEXT("Server is listening at %s:%d\n"), ip, port);
	setServerStatusGui(serverStatusGuiEdt, _T("Listening"));
	appendServerLog(serverLogGuiEdt, buf);

	// main server loop
	while (!stopCameraServer->load()) 
	{
	}
	OutputDebugString(_T("Out of loop \n"));

	//setServerStatusGui(serverStatusGuiEdt, _T("Stopped"));
	OutputDebugString(_T("Sent stopped status\n"));
	//appendServerLog(serverLogGuiEdt, TEXT("Server stopped.\n"));

	OutputDebugString(_T("Finished line\n"));
}

void CameraServer::setServerStatusGui(CEdit* serverStatusGuiEdt, CString status)
{
	serverStatusGuiEdt->SetWindowText(status);
}

void CameraServer::appendServerLog(CEdit* serverLogGuiEdt, CString text)
{
	int idx = GetWindowTextLength(*serverLogGuiEdt);
	CT2A ascii(text, CP_UTF8);
	SendMessage(*serverLogGuiEdt, EM_SETSEL, (WPARAM)idx, (LPARAM)idx);
	SendMessage(*serverLogGuiEdt, EM_REPLACESEL, 0, (LPARAM) ascii.m_psz);
}

