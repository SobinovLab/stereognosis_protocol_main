#pragma once

#include <atomic>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "cameras_messages.grpc.pb.h"
#include "cameras_messages.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

class CameraCommunicatorSClient 
{
public:
	CameraCommunicatorSClient(std::shared_ptr<Channel> channel) : stub_(CameraCommunicatorS::NewStub(channel)) {
	}

	bool sendFramerate(const double framerate);
	bool sendRecordingPeriod(const double recordingPeriod);
	bool sendReferenceCamera(const int serial);
	bool sendDirectory(const CString directory);
	bool sendGain(const CString gain);
	bool sendExposure(const CString exposure);

	bool prepareRecording();
	bool startRecording();
	bool captureSingleFrame();

	INT32 lastCode = 0;
	CString* lastDescritpion;

private:
	std::unique_ptr<CameraCommunicatorS::Stub> stub_;
};

class CameraClient
{
public:
	CameraClient();
	virtual ~CameraClient();

	virtual void connect_f();
	virtual void disconnect_f();

	virtual void sendFramerate(const double framerate);
	virtual void sendRecordingPeriod(const double recordingPeriod);
	virtual void sendReferenceCamera(const int serial);
	virtual void sendDirectory(const CString directory);
	virtual void sendGain(const CString gain);
	virtual void sendExposure(const CString exposure);

	virtual void syncTime();

	virtual void prepareRecording();
	virtual void startRecording();
	virtual void captureSingleFrame();

	virtual bool isConnected();

	CString server_ip;
	long port;

	CEdit* clientStatusGuiEdt;
	CEdit* clientLogGuiEdt;

private:
	CameraCommunicatorSClient* ccsc;

	void setClientStatusGui(CString status);
	void appendClientLog(CString text);
};

