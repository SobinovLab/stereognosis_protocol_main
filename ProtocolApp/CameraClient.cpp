#include "CameraClient.h"

CameraClient::CameraClient()
{
	server_ip = "localhost";
	port = 60000;

	disconnect.store(false);
	ccsc = nullptr;
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

void CameraClient::connect_f()
{
	CString serverAddress;
	serverAddress.Format(_T("%s:%d"), server_ip, port);

	setClientStatusGui(_T("Connecting..."));
	CStringA buf;
	buf.Format(TEXT("Connecting to server at %s\n"), serverAddress);
	appendClientLog(buf);

	ccsc = new CameraCommunicatorSClient(grpc::CreateChannel(std::string(serverAddress), 
		grpc::InsecureChannelCredentials()));

	setClientStatusGui(_T("Connected"));
	appendClientLog(_T("Connected to server.\n"));
}

void CameraClient::disconnect_f()
{
	setClientStatusGui(_T("Disconnected"));
	appendClientLog(_T("Disconnected from server.\n"));
}

void CameraClient::sendFramerate(const double framerate)
{
	appendClientLog(_T("Sending framerate.\n"));
	if (ccsc) {
		if (ccsc->sendFramerate(framerate))
			appendClientLog(_T("Success.\n"));
		else
			appendClientLog(_T("Failure.\n"));
	}

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

bool CameraCommunicatorSClient::SendSetFramerate(const SetFramerateRequest& setFramerateRequest, SetFramerateResponse* setFramerateResponse)
{
	ClientContext context;
	Status status = stub_->SetFramerate(&context, setFramerateRequest, setFramerateResponse);
	if (!status.ok()) {
		OutputDebugString(_T("Set rpc failed.\n"));
		return false;
	}
	CString buf;
	buf.Format(_T("Respond code %d, desc %s\n"), setFramerateResponse->code(), setFramerateResponse->description());
	OutputDebugString(buf);

	return true;
}

bool CameraCommunicatorSClient::sendFramerate(const double framerate)
{
	SetFramerateRequest sfreq;
	sfreq.set_framerate(framerate);
	sfreq.set_desc("single");
	
	SetFramerateResponse sfres;

	return this->SendSetFramerate(sfreq, &sfres);
}
