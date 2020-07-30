#include "CameraClient.h"

CameraClient::CameraClient()
{
	server_ip = "localhost";
	port = 60000;

	ccsc = nullptr;
}

CameraClient::~CameraClient()
{
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
	delete ccsc;	ccsc = nullptr;

	setClientStatusGui(_T("Disconnected"));
	appendClientLog(_T("Disconnected from server.\n"));
}

void CameraClient::sendFramerate(const double framerate)
{
	appendClientLog(_T("Sending framerate. "));
	if (ccsc) {
		if (ccsc->sendFramerate(framerate))
			appendClientLog(_T("Success.\n"));
		else
			appendClientLog(_T("Failure.\n"));
	}

}

void CameraClient::sendRecordingPeriod(const double recordingPeriod)
{
	appendClientLog(_T("Sending recording period. "));
	if (ccsc) {
		if (ccsc->sendRecordingPeriod(recordingPeriod))
			appendClientLog(_T("Success.\n"));
		else
			appendClientLog(_T("Failure.\n"));
	}
}

void CameraClient::sendReferenceCamera(const int serial)
{
	appendClientLog(_T("Sending reference camera serial. "));
	if (ccsc) {
		if (ccsc->sendReferenceCamera(serial))
			appendClientLog(_T("Success.\n"));
		else
			appendClientLog(_T("Failure.\n"));
	}
}

void CameraClient::sendDirectory(const CString directory)
{
	appendClientLog(_T("Sending directory. "));
	if (ccsc) {
		if (ccsc->sendDirectory(directory))
			appendClientLog(_T("Success.\n"));
		else
			appendClientLog(_T("Failure.\n"));
	}
}

void CameraClient::sendGain(const CString gain)
{
	appendClientLog(_T("Sending gain. "));
	if (ccsc) {
		if (ccsc->sendGain(gain))
			appendClientLog(_T("Success.\n"));
		else
			appendClientLog(_T("Failure.\n"));
	}
}

void CameraClient::sendExposure(const CString exposure)
{
	appendClientLog(_T("Sending exposure. "));
	if (ccsc) {
		if (ccsc->sendExposure(exposure))
			appendClientLog(_T("Success.\n"));
		else
			appendClientLog(_T("Failure.\n"));
	}
}

void CameraClient::syncTime()
{
	appendClientLog(_T("Syncing time. "));
	// TODO (with params)
	appendClientLog(_T("Time synchronization not implemented.\n"));
}

void CameraClient::prepareRecording()
{
	appendClientLog(_T("Preparing recording. "));
	if (ccsc) {
		if (ccsc->prepareRecording()) {
			appendClientLog(_T("Success.\n"));
			setClientStatusGui(_T("Armed"));
		}
		else
			appendClientLog(_T("Failure.\n"));
	}
	else {
		appendClientLog(_T("Not connected.\n "));
	}
}

void CameraClient::startRecording()
{
	appendClientLog(_T("Starting recording. "));
	if (ccsc) {
		if (ccsc->startRecording()) {
			appendClientLog(_T("Success.\n"));
			setClientStatusGui(_T("Recording"));
		}
		else
			appendClientLog(_T("Failure.\n"));
	}
	else {
		appendClientLog(_T("Not connected.\n "));
	}
}

void CameraClient::captureSingleFrame()
{
	appendClientLog(_T("Requesting capture single frame. "));
	if (ccsc) {
		if (ccsc->captureSingleFrame()) {
			appendClientLog(_T("Success.\n"));
		}
		else
			appendClientLog(_T("Failure.\n"));
	}
	else {
		appendClientLog(_T("Not connected.\n "));
	}
}

void CameraClient::breakRecording()
{
	appendClientLog(_T("Requesting to stop recording. "));
	if (ccsc) {
		if (ccsc->breakRecording()) {
			appendClientLog(_T("Success.\n"));
		}
		else
			appendClientLog(_T("Failure.\n"));
	}
	else {
		appendClientLog(_T("Not connected.\n "));
	}
}

bool CameraClient::isConnected()
{
	if (ccsc)
		return true;
	return false;
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

bool CameraCommunicatorSClient::sendFramerate(const double framerate)
{
	SetFramerateRequest sfreq;
	sfreq.set_framerate(framerate);
	sfreq.set_desc("single");
	
	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->SetFramerate(&context, sfreq, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}

bool CameraCommunicatorSClient::sendRecordingPeriod(const double recordingPeriod)
{
	SetRecordingPeriodRequest srp;
	srp.set_time(recordingPeriod);

	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->SetRecordingPeriod(&context, srp, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}

bool CameraCommunicatorSClient::sendReferenceCamera(const int serial)
{
	SetReferenceCameraRequest src;
	src.set_serial(serial);

	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->SetReferenceCamera(&context, src, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}

bool CameraCommunicatorSClient::sendDirectory(const CString directory)
{
	SetDirectoryRequest src;
	src.set_directory(directory);

	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->SetDirectory(&context, src, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}

bool CameraCommunicatorSClient::sendGain(const CString gain)
{
	SetGainRequest src;
	if (gain == _T("auto")) {
		src.set_gain_type(1);
		src.set_gain(0);
	}
	else {
		src.set_gain_type(0);
		src.set_gain(_tstof(gain));
	}

	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->SetGain(&context, src, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}

bool CameraCommunicatorSClient::sendExposure(const CString exposure)
{
	SetExposureRequest src;
	if (exposure == _T("auto")) {
		src.set_exposure_type(1);
		src.set_exposure(0);
	}
	else {
		src.set_exposure_type(0);
		src.set_exposure(_tstof(exposure));
	}

	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->SetExposure(&context, src, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}

bool CameraCommunicatorSClient::prepareRecording()
{
	SimpleRequest srq;
	srq.set_code(0);
	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->PrepareRecording(&context, srq, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}

bool CameraCommunicatorSClient::startRecording()
{
	SimpleRequest srq;
	srq.set_code(0);
	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->StartRecording(&context, srq, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}

bool CameraCommunicatorSClient::captureSingleFrame()
{
	SimpleRequest srq;
	srq.set_code(0);
	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->CaptureSingleImage(&context, srq, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}

bool CameraCommunicatorSClient::breakRecording()
{
	SimpleRequest srq;
	srq.set_code(0);
	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->BreakRecording(&context, srq, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;
}
