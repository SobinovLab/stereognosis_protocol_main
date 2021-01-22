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
    void move(const int& moveCounts, const int& speed, const int& accel);

public:
    Node(sFnd::INode& node, std::string type);
    ~Node(void);

    // homes-calibrates the motors
    void home();
    // general move function
    void moveHigh(const int& position, const int& velLevel, const int& accLevel);
    // overload with default velocity and acceleration
    void moveHigh(const int& position);
    // retreat to the starting position - call moveHigh
    void retreat();
    // stops the current movement
    void stop();

    // run on creation and destruction
    void enable();
    void disable();

    void handleAlerts();

    // accessory functions
    void printDetails();

    // homing and moving
    double action_timeout = 10;  // seconds

};


class MotorAPI {
private:
    bool initializedCorrectly = false;  // set by constructor

    sFnd::SysManager* m_manager = nullptr;
    
    std::vector<std::reference_wrapper<sFnd::IPort>> m_ports;
    std::vector<std::reference_wrapper<Node>> m_nodes;

public:

    MotorAPI();
    ~MotorAPI();

    // main control functions
    int home();
    int retreat();
    int move(std::vector<int> positions);

    bool wasInitializedCorrectly();

    void setActionTimeout(double timeSecs);

};

