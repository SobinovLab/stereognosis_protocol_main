#include "stdafx.h"
#include "TouchSensorClient.h"

using namespace TekscanServerNamespace;

bool TouchSensorSClient::startRecording(int trialNum)
{
	StartRecordingRequest srq;
	srq.set_trialnum(trialNum);
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


bool TouchSensorSClient::setTimestamp(int timestamp)
{
	SetTimestampRequest strq;
	strq.set_timestamp(timestamp);
	SimpleResponse sr;
	ClientContext context;

	Status status = stub_->SetTimestamp(&context, strq, &sr);
	if (!status.ok()) {
		return false;
	}

	lastCode = sr.code();
	lastDescritpion = new CString(sr.description().c_str());

	return true;

}



bool TouchSensorSClient::breakRecording(std::atomic<int>* result)
{
	result->store(-1);
	SimpleRequest srq;
	srq.set_code(0);
	BreakRecordingResponse sr;
	ClientContext context;

	Status status = stub_->BreakRecording(&context, srq, &sr);
	if (!status.ok()) {
		return false;
	}

	result->store(sr.successlevel());

	return true;
}

bool TouchSensorSClient::getForce(std::atomic<double>* leftForce,
                                  std::atomic<double>* rightForce,
                                  std::atomic<double>* topLeftForce,
                                  std::atomic<double>* bottomLeftForce,
                                  std::atomic<double>* topRightForce,
                                  std::atomic<double>* bottomRightForce) {
	leftForce->store(-1);
	rightForce->store(-1);
	topLeftForce->store(-1);
	bottomLeftForce->store(-1);
	topRightForce->store(-1);
	bottomRightForce->store(-1);

	SimpleRequest srq;
	srq.set_code(0);

	ForceResponse fr;
	ClientContext context;

	Status status = stub_->GetForce(&context, srq, &fr);
	if (!status.ok()) {
		return false;
	}

	leftForce->store(fr.leftforce());
    rightForce->store(fr.rightforce());
    topLeftForce->store(fr.topleftforce());
    bottomLeftForce->store(fr.bottomleftforce());
    topRightForce->store(fr.toprightforce());
    bottomRightForce->store(fr.bottomrightforce());

	return true;
}

bool TouchSensorSClient::syncMessageTrialStart()
{
	Empty eq;
	Empty er;
	ClientContext context;

	Status status = stub_->SyncMessageTrialStart(&context, eq, &er);
	if (!status.ok()) {
		return false;
	}
	return true;
}

bool TouchSensorSClient::syncMessageTrialEnd()
{
	Empty eq;
	Empty er;
	ClientContext context;

	Status status = stub_->SyncMessageTrialEnd(&context, eq, &er);
	if (!status.ok()) {
		return false;
	}
	return true;
}

TouchSensorClient::TouchSensorClient()
{
	server_ip = "localhost";
	port = 60000;

	tssc = nullptr;
}

TouchSensorClient::~TouchSensorClient()
{
}

void TouchSensorClient::connect_f()
{
	std::string serverAddress(server_ip);
	serverAddress += ":" + std::to_string(port);
	//CString serverAddress = server_ip;
	//serverAddress.Format(_T("%s:%d"), server_ip, port);

	CString buf = "";
	buf.Format(TEXT("Connecting to server at %s\n"), serverAddress.c_str());
	appendClientLog(buf);

	tssc = new TouchSensorSClient(grpc::CreateChannel(serverAddress,
		grpc::InsecureChannelCredentials()));

	appendClientLog(_T("Connected to server.\n"));
}

void TouchSensorClient::disconnect_f()
{
	delete tssc;	tssc = nullptr;

	appendClientLog(_T("Disconnected from server.\n"));
}

bool TouchSensorClient::isConnected()
{
	if (tssc)
		return true;
	return false;
}


void TouchSensorClient::setTimestamp(int timestamp)
{
	appendClientLog(_T("Setting timestamp from main protoc."));
	if (tssc) {
		if (tssc->setTimestamp(timestamp)) {
			appendClientLog(_T("Success.\n"));
		}
		else
			appendClientLog(_T("Failure.\n"));
	}
	else {
		appendClientLog(_T("Not connected.\n "));
	}
}


void TouchSensorClient::startRecording(int trialNum)
{
	appendClientLog(_T("Starting recording. "));
	if (tssc) {
		if (tssc->startRecording(trialNum)) {
			appendClientLog(_T("Success.\n"));
		}
		else
			appendClientLog(_T("Failure.\n"));
	}
	else {
		appendClientLog(_T("Not connected.\n "));
	}
}

void TouchSensorClient::breakRecording(std::atomic<int>* result)
{
	appendClientLog(_T("Requesting to stop recording. "));
	if (tssc) {
		if (tssc->breakRecording(result)) {
			CStringA buf;
			buf.Format(TEXT("Success: %d\n"), result->load());
			appendClientLog(buf);
		}
		else
			appendClientLog(_T("Failure.\n"));
	}
	else {
		appendClientLog(_T("Not connected.\n "));
	}
}

void TouchSensorClient::getForce(std::atomic<double>* leftForce,
                                 std::atomic<double>* rightForce,
                                 std::atomic<double>* topLeftForce,
                                 std::atomic<double>* bottomLeftForce,
                                 std::atomic<double>* topRightForce,
                                 std::atomic<double>* bottomRightForce) {
	if (tssc) {
    tssc->getForce(leftForce, rightForce, topLeftForce, bottomLeftForce,
                         topRightForce, bottomRightForce);
	}
}

void TouchSensorClient::syncMessageTrialStart()
{
	if (tssc)
		tssc->syncMessageTrialStart();
}

void TouchSensorClient::syncMessageTrialEnd()
{
	if (tssc)
		tssc->syncMessageTrialEnd();
}

void TouchSensorClient::appendClientLog(CString text)
{
	if (clientLogGuiEdt) {
		int idx = GetWindowTextLength(*clientLogGuiEdt);
		CT2A ascii(text, CP_UTF8);
		SendMessage(*clientLogGuiEdt, EM_SETSEL, (WPARAM)idx, (LPARAM)idx);
		SendMessage(*clientLogGuiEdt, EM_REPLACESEL, 0, (LPARAM)ascii.m_psz);
	}
}
