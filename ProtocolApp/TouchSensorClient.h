#pragma once

#include <atomic>

#include <string>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#pragma warning(push, 3) 
#include "tekscan_server.grpc.pb.h"
#include "tekscan_server.pb.h"
#pragma warning(pop)

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

class TouchSensorSClient 
{
public:
	TouchSensorSClient(std::shared_ptr<Channel> channel) : stub_(TekscanServerNamespace::TekscanServer::NewStub(channel)), lastDescritpion(new CString("")){
	}

	bool startRecording(int trialnum);
	bool breakRecording(std::atomic<int>* result);
	bool checkSuccess(std::atomic<int>* result);

	INT32 lastCode = 0;
	CString* lastDescritpion;

private:
	std::unique_ptr<TekscanServerNamespace::TekscanServer::Stub> stub_;
};

class TouchSensorClient
{
public:
	TouchSensorClient();
	virtual ~TouchSensorClient();

	virtual void connect_f();
	virtual void disconnect_f();
	virtual bool isConnected();

	virtual void startRecording(int trialnum);
	virtual void breakRecording(std::atomic<int>* result);
	virtual void checkSuccess(std::atomic<int>* result);

	CString server_ip;
	long port;

	CEdit* clientLogGuiEdt;

private:
	TouchSensorSClient* tssc;

	void appendClientLog(CString text);
};

