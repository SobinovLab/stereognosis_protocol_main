#include <vector>
#include <string>
#include <pubSysCls.h>
#include "Times.h"
#include "Logger.h"

#pragma comment(lib, "sFoundation20.lib")


class Convertor {
private:
    double min_level;
    double max_level;
    double max_motor_val;

public:
    Convertor(const double _min_level, const double _max_level, const double _max_motor_val);
    ~Convertor();

    int convert(const int val);
    int convert(const double val);
};


class Node {
private:
    sFnd::INode& m_node;
    Node() = delete;
    std::string m_name;

    // conversion variables
    Convertor* pos;
    Convertor* vel;
    Convertor* acc;

    int retreat_position;
    int default_vel;
    int default_acc;

    // internal control functions
    void m_move(const int& moveCounts, const int& speed, const int& accel);
    int m_initiateMove(const int& moveCounts, const int& speed, const int& accel);

public:
    Node(sFnd::INode& node, const int index);
    ~Node(void);

    //-------- main functions used outside
    // homes-calibrates the motors
    int home();
    // general move function --  efectively deprecated and not used, but OK example. see initateMove functions
    void move(const int& position, const int& velLevel, const int& accLevel);
    // overload with default velocity and acceleration
    void move(const int& position);
    // retreat to the starting position - call moveHigh
    void retreat();
    // stops the current movement
    void stop();

    // initiate async movement
    int initiateMove(const int& position, const int& velLevel, const int& accLevel);
    int initiateMove(const int& position);
    int initiateRetreat();

    //-------- status
    // run on creation and destruction
    int enable();
    void disable();

    void clearAlertsNodeStops();
    void handleAlerts();

    bool isMoveDone();

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
    std::vector<std::reference_wrapper<Node>> m_nodes;

    double action_timeout = 10;

    std::mutex mtx;
public:

    MotorAPI();
    ~MotorAPI();

    // main control functions
    int home();
    int retreat();
    int move(std::vector<int> positions, std::atomic<bool> *stopTrial, std::atomic<bool>* stopProtocol);
    void stop();  // thread-safe with move and retreat

    bool wasInitializedCorrectly();

    void setActionTimeout(double timeSecs);

};

