#ifndef PMJ_KINOVA_ARM_LIB
#define PMJ_KINOVA_ARM_LIB 1

#include <iostream>
#include <memory>
#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "armMessages.grpc.pb.h"
#include "armMessages.pb.h"

#define DEFAULT_IP "localhost:50051"


class KinovaArmClient 
{
public:
    KinovaArmClient()
    {
        stub = armCommunication::NewStub(grpc::CreateChannel(DEFAULT_IP, grpc::InsecureChannelCredentials()));
    }
    KinovaArmClient(std::string IP)
    {
        stub = armCommunication::NewStub(grpc::CreateChannel(IP, grpc::InsecureChannelCredentials()));
    }

    int getArmStatus()
    {
        grpc::ClientContext context;
        statusRequest req;
        statusResponse resp;
        req.set_flag(1);
        grpc::Status status = stub->armStatus(&context, req, &resp);
        if(!status.ok())
        {
            std::cout << "GRPC getArmStatus request failed. Error: " << status.error_message() << std::endl;
            return -20;
        }
        std::cout << "Arm Status: " <<  resp.flag() << std::endl;
        return resp.flag();
    }

    int goToHome()
    {
        grpc::ClientContext context;
        moveHome homeReq;
        moveResponse moveResp;

        grpc::Status status = stub->armHome(&context, homeReq, &moveResp);
        if(!status.ok())
        {
            std::cout << "GRPC goToHome request failed. Error: " << status.error_message() << std::endl;
            return -20;
        }
        std::cout << "Home Resp: " << moveResp.responsecode() << std::endl;
        return moveResp.responsecode();
    }

    int moveToPosition(double X, double Y, double Z, double theta, double phi, double chi, double width)
    {
        grpc::ClientContext context;
        moveArm moveReq;
        moveResponse moveResp;

        moveReq.set_x(X/1000.0);
        moveReq.set_y(Y/1000.0);
        moveReq.set_z(Z/1000.0);
        moveReq.set_theta(theta);
        moveReq.set_phi(phi);
        moveReq.set_chi(chi);
        moveReq.set_width(width);

        grpc::Status status = stub->armControl(&context, moveReq, &moveResp);
        if(!status.ok())
        {
            std::cout << "GRPC moveToPosition request failed. Error: " << status.error_message() << std::endl;
            return -20;
        }
        std::cout << "Home Resp: " << moveResp.responsecode() << std::endl;
        return moveResp.responsecode();
    }
    void getTorques()
    {
        //TODO: Future feature
        return;
    }

private:
    std::unique_ptr<armCommunication::Stub> stub;

};

#endif