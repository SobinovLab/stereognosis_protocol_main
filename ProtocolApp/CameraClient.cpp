#include "CameraClient.h"

CameraClient::CameraClient()
{
	server_ip = "localhost";
	port = 60000;

	disconnect.store(false);
}

CameraClient::~CameraClient()
{
}

void CameraClient::run()
{
	setClientStatusGui(_T("Connecting..."));
	CStringA buf;
	buf.Format(TEXT("Connecting to server at %s:%d\n"), server_ip, port);
	appendClientLog(buf);

	// TODO connect


	setClientStatusGui(_T("Connected"));
	appendClientLog(_T("Connected to server.\n"));

	// TODO listen to messages
	while (!disconnect.load())
	{

	}

	//setClientStatusGui(_T("Disconnected"));
	//appendClientLog(_T("Disconnected from server.\n"));
}

void CameraClient::setClientStatusGui(CString status)
{
	if (clientStatusGuiEdt)
		clientStatusGuiEdt->SetWindowText(status);
}

void CameraClient::appendClientLog(CString text)
{
	if (clientLogGuiEdt) {
		int idx = GetWindowTextLength(*clientLogGuiEdt);
		CT2A ascii(text, CP_UTF8);
		SendMessage(*clientLogGuiEdt, EM_SETSEL, (WPARAM)idx, (LPARAM)idx);
		SendMessage(*clientLogGuiEdt, EM_REPLACESEL, 0, (LPARAM)ascii.m_psz);
	}
}
