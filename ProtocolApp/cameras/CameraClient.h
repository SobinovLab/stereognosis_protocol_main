#pragma once

#include <atomic>

#pragma warning( push, 0 )
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "cameras_messages.grpc.pb.h"
#include "cameras_messages.pb.h"
#pragma warning( pop )

class CameraCommunicatorSClient 
{
public:
	CameraCommunicatorSClient(std::shared_ptr<grpc::Channel> channel) : stub_(CameraCommunicatorS::NewStub(channel)) {
	}

	bool sendFramerate(const double framerate);
	bool sendRecordingPeriod(const double recordingPeriod);
	bool sendReferenceCamera(const int serial);
	bool sendDirectory(const CString directory);
	bool sendGain(const CString gain);
	bool sendExposure(const CString exposure);

	bool prepareRecording();
	bool startRecording(int trialNumber, int* success);
	bool captureSingleFrame(int* success);
	bool breakRecording();
	bool areYouDoneSaving(int *success);

	INT32 lastCode = 0;
	CString* lastDescritpion;

private:
	std::unique_ptr<CameraCommunicatorS::Stub> stub_;
};

class CameraClient
{
public:
	CameraClient();
	~CameraClient();

	void connect_f();
	void disconnect_f();

	void sendFramerate(const double framerate);
	void sendRecordingPeriod(const double recordingPeriod);
	void sendReferenceCamera(const int serial);
	void sendDirectory(const CString directory);
	void sendGain(const CString gain);
	void sendExposure(const CString exposure);

	void prepareRecording();
	bool startRecording(int trialNumber, int* success);
	bool captureSingleFrame(int* success);
	void breakRecording();
	bool areYouDoneSaving(int* success);  // return true on successful request

	bool isConnected();

	CString server_ip;
	long port;

	// 1: not connected; 2: message failure
	std::atomic<int> lastErrorCode = 0;

	CEdit* clientLogGuiEdt;

private:
	CameraCommunicatorSClient* ccsc;

	void appendClientLog(CString text);
};

