#include <vector>
#include <string>
#include <pubSysCls.h>
#include "Times.h"
#include "Logger.h"

#pragma comment(lib, "sFoundation20.lib")


class Convertor {
private:
    // Order of operations:
    // change the direction of input vs output
    double sign;
    // added to the input
    double in_offset;
    // multiply
    double in_to_out_factor;

    // bound range
    int min_out;
    int max_out;

public:
    Convertor(const int _min_out, const int _max_out, const double _in_offset, const double _in_to_out_factor, const double sign=1);
    ~Convertor();

    int convert(const int val);
    int convert(const double val);
};


class Node {
private:
    sFnd::INode* m_node;
    Node() = delete;
    std::string m_name;

    // conversion variables
    Convertor* pos;
    Convertor* vel;
    Convertor* acc;

    double retreat_position;
    int default_vel;
    int default_acc;

    // internal control functions
    void m_move(const int& moveCounts, const int& speed, const int& accel);
    int m_initiateMove(const int& moveCounts, const int& speed, const int& accel);

public:
    Node(sFnd::INode* node, const int index);
    ~Node(void);

    //-------- main functions used outside
    // homes-calibrates the motors
    int home();
    // general move function --  efectively deprecated and not used, but OK example. see initateMove functions
    void move(const double& position, const int& velLevel, const int& accLevel);
    // overload with default velocity and acceleration
    void move(const double& position);
    // retreat to the starting position - call moveHigh
    void retreat();
    // stops the current movement
    void stop();

    // initiate async movement
    int initiateMove(const double& position, const int& velLevel, const int& accLevel);
    int initiateMove(const double& position);
    int initiateRetreat();

    //-------- status
    // run on creation and destruction
    int enable();
    void disable();

    void clearAlertsNodeStops();
    void handleAlerts();

    bool isMoveDone();

    bool wasHomed();

    //-------- accessory functions
    void printDetails();

    // seconds for homing and moving
    double action_timeout = 10;

};


class MotorAPI {
private:
    bool initializedCorrectly = false;  // set by constructor

    sFnd::SysManager* m_manager = nullptr;
    
    std::vector<std::reference_wrapper<sFnd::IPort>> m_ports;
    std::vector<Node> m_nodes;

    double action_timeout = 10;

    std::mutex mtx;
public:

    MotorAPI();
    ~MotorAPI();

    // main control functions
    int home();
    int retreat();
    int move(std::vector<double> positions, std::atomic<bool> *stopTrial, std::atomic<bool>* stopProtocol);
    void stop();  // thread-safe with move and retreat

    bool wasInitializedCorrectly();
    bool wereHomed();

    void setActionTimeout(double timeSecs);

};

