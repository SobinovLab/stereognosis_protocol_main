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

	bool SendSetFramerate(const SetFramerateRequest& setFramerateRequest, SetFramerateResponse* setFramerateResponse);

	bool sendFramerate(const double framerate);

private:
	std::unique_ptr<CameraCommunicatorS::Stub> stub_;
};

class CameraClient
{
public:
	CameraClient();
	virtual ~CameraClient();

	virtual void run();

	virtual void connect_f();
	virtual void disconnect_f();
	virtual void sendFramerate(const double framerate);

	CString server_ip;
	long port;

	std::atomic<bool> disconnect;

	CEdit* clientStatusGuiEdt;
	CEdit* clientLogGuiEdt;

private:
	CameraCommunicatorSClient* ccsc;

	void setClientStatusGui(CString status);
	void appendClientLog(CString text);
};

