#pragma once

#include <atomic>

#include <string>

#pragma warning( push, 0 )
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

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
	bool startRecording(int trialNum);
	bool setTimestamp(int timestamp);
	bool breakRecording(std::atomic<int>* result);
	bool getForce(
		std::atomic<double>* leftForce, 
		std::atomic<double>* rightForce,
		std::atomic<double>* topLeftForce,
		std::atomic<double>* bottomLeftForce,
		std::atomic<double>* topRightForce,
		std::atomic<double>* bottomRightForce);

	bool syncMessageTrialStart();
	bool syncMessageTrialEnd();

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

	void setTimestamp(int timestamp);
	void startRecording(int trialNum);
	virtual void breakRecording(std::atomic<int>* result);
    virtual void getForce(
		std::atomic<double>* leftForce,
		std::atomic<double>* rightForce,
		std::atomic<double>* topLeftForce,
		std::atomic<double>* bottomLeftForce,
		std::atomic<double>* topRightForce,
		std::atomic<double>* bottomRightForce);

	void syncMessageTrialStart();
	void syncMessageTrialEnd();

	CString server_ip;
	long port;

	CEdit* clientLogGuiEdt;

private:
	TouchSensorSClient* tssc;

	void appendClientLog(CString text);
};

