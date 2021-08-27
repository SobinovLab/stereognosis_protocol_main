/*
* Api controls movement around a number of axes. Each axis can be moved by a number of motors.
* 
* NOTE:
*   Parallel group can be driven simultaneously only if they are on the same port.
*/
#pragma once

#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <memory>

#include <nlohmann/json.hpp>
#include <pubSysCls.h>

#include "Convertor.h"
#include "Times.h"
#include "Logger.h"

#pragma comment(lib, "sFoundation20.lib")


enum class TEKNIC_MOTOR_API_CODE {
    OK = 0,
    MOTOR_DISABLED_BY_CONFIG = 1,
    MOTOR_ALREADY_ENABLED = 2,
    NO_MOTORS_ON_AXIS = 3,

    ACTION_TIMEOUT = -1,
    MOTOR_HOMING_INVALID = -2,
    TEKNIC_ERROR = -3,
    COULD_NOT_ENABLE_MOTOR = -4,
    ACTION_COMPLETED_INCORRECTLY = -5,
    DOES_NOT_SUPPORT_FUNCTION = -6,
    USER_INTERRUPT = -7,
    RANGE_INCONSISTENT = -8,
    INITIALIZATION_ERROR = -9,
    ENGAGED = -10,
    JSON_PARSING_ERROR = -11,
};


class Motor {
public:
    Motor(sFnd::INode& node, const nlohmann::json settings);
    ~Motor(void);

    // ------- general move functions - in units of AXIS
    // for all these functions position in units of object movement, e.g. mm or deg
    // velocity and acceleration 0-100% of max value at 75 VDC
    // There are 2 types of functions
    //      to use with basic (ELSB) motors. These can be used on all and any motors. Not recommended
    //          if the motors are mechanically linked. Simultaneous homing is not written in here.
    //      to use with advanced (ELSA) motors. These should be used when ALL motors on the axis are advanced.

    // to use on ELSB (basic) not-advanced motors 
    TEKNIC_MOTOR_API_CODE moveBasic(const double& position, const double& velocity, const double& acceleration);

    // to use on ELSB (basic) not-advanced motors 
    TEKNIC_MOTOR_API_CODE homeBasic();
    
    // async basic movement. enables motor, handles alerts and set limits
    TEKNIC_MOTOR_API_CODE prepareMoveBasic(const double& velocity, const double& acceleration);
    // async basic movement. Expects the motor to be enabled and ready to move
    TEKNIC_MOTOR_API_CODE startMoveBasic(const double& position);

    // Advanced triggered movements
    TEKNIC_MOTOR_API_CODE prepareMoveAdvanced(const double& position, const double& velocity, const double& acceleration);

    // advanced homing
    TEKNIC_MOTOR_API_CODE prepareHomeAdvanced(const double& velocity, const double& acceleration);
    // sets the internal position counter to zero
    void zeroMotor();
    // sets the HOMED flag
    void markMotorHomed();

    // stops the current movement  -- thread-safe
    void stop();

    // handles alerts, clears node stops, attempts to enable
    TEKNIC_MOTOR_API_CODE prepareForAction();

    // triggers an advanced move, expects the node to be enabled and advanced
    TEKNIC_MOTOR_API_CODE triggerSameGroupMovement();

    // -------- status
    // sends message to motor to enable 
    TEKNIC_MOTOR_API_CODE enable();
    // sends message to motor to disable
    TEKNIC_MOTOR_API_CODE disable();
    // User-config controlled state - should not try to use a disabled motor,
    // Changing the variable will lead to undefined behavior, as well as using the disabled object
    // for anything but printing reports and sending "stop" signals.
    const bool isDisabledByConfig();
    void clearMoves();

    // thread-safe
    bool isMoveDone();
    bool wasHomed();
    // thread-safe
    bool isHoming();
    bool isAdvanced();
    bool isInPosition(const double& position);

    // thread-safe
    double getTorque();

    double timeout_for_enable = 60;
    double basic_action_timeout = 60;  // homing and moving

    // Sets an advanced parallel group. Should not be called on a non-advanced motor
    void setParallelGroup(const int pg);

    //-------- accessory functions
    // good for logging
    std::string getDetails();
    std::string model;
    std::string name = "UNSET";
    // shorter, good for UI print
    std::string getReport();

    // range
    void setRange(const std::vector<double> v);
    double forceInRange(const double position);
    std::vector<double> getPosRange();
    void removeRange();

    static const int POSITION_COUNT_THRESHOLD = 50;

    int getCurrentPosition();

private:
    // reference to the control structure
    sFnd::INode& m_node;
    Motor() = delete;
    Motor(Motor &) = delete;
    // User-config controlled state - should not try to use a disabled motor,
    // Changing the variable will lead to undefined behavior, as well as using the disabled object
    // for anything but printing reports and sending "stop" signals.
    bool disabled = false;

    // conversion variables
    Convertor pos;
    Convertor vel;
    Convertor acc;

    // -------- internal control functions - in units of the MOTOR
    //  counts, RPM, RPM/s
    TEKNIC_MOTOR_API_CODE m_moveBasic(const int& moveCounts, const double& velocity, const double& acceleration);

    // basic parallel control of motors
    TEKNIC_MOTOR_API_CODE m_prepareMoveBasic(const double velocity, const double acceleration);
    TEKNIC_MOTOR_API_CODE m_startMoveBasic(const int moveCounts);

    // advanced parallel control of motors
    TEKNIC_MOTOR_API_CODE m_prepareMoveAdvanced(const int moveCounts, const double velocity, const double acceleration);

    // advanced parallel homing of motors
    TEKNIC_MOTOR_API_CODE m_prepareHomeAdvanced(const double velocity, const double acceleration);

    bool m_isInPosition(const int targetCounts);

    // -------- thread-safe node function call wrappers
    // Only functions that are frequently called and can be interrupted by STOP are wrapped
    bool motionIsReady();
    // isHoming
    // isMoveDone

    // -------- alerts and stuff
    void clearNodeStops();
    void handleAlerts();
};


class Axis {
public:
    Axis(nlohmann::json axis_json);
    ~Axis();

    // main parameters and idetifiers
    std::string name;
    int port;  // serial port number
    bool retreat_axis;
    std::string units;

    // movement-related parameters
    double default_vel;
    double default_acc;
    double neutral_pos;
    std::vector<double> range;

    // homing parameters
    int homing_direction;
    double homing_offset_in_units;
    double homing_torque_threshold_percent;
    double homing_timeout;  // seconds

    // other parameters
    double basic_action_timeout = 60;  // homing and moving

    // adds a properly created motor object to the motors vector
    TEKNIC_MOTOR_API_CODE addMotor(sFnd::INode& node, nlohmann::json motor_json);
    //-------- main functions - move all motors on the axis
    // If any of the motors on the axis are basic, they cannot be physically coupled
    // a new parallel basic homing function should be developed to allow that behavior
    // move can move several basic motors on the axis in parallel, but they will not be launched with
    // the same precision as advanced ones.

    // homes-zeros the motors
    TEKNIC_MOTOR_API_CODE homeBasic();

    // general move function
    TEKNIC_MOTOR_API_CODE moveBasic(const double& position, const double& velocity, const double& acceleration);
    // overload with default velocity and acceleration
    TEKNIC_MOTOR_API_CODE moveBasic(const double& position);

    // advanced parallel triggered movement
    TEKNIC_MOTOR_API_CODE prepareAdvancedMovement(const double& position, const double& velocity, const double& acceleration);
    TEKNIC_MOTOR_API_CODE prepareAdvancedMovement(const double& position);

    // advanced parallel homing
    TEKNIC_MOTOR_API_CODE homeAdvanced(int freeGroupNumber);

    // stops the current movement and clear move buffer
    void stop();

    // --------- status
    void disable();
    void clearMoves();

    bool wereAllHomed();
    bool isAllMoveDone();  // ALL motors done moving
    bool areAnyHoming();  // ANY motors still homing
    bool areAllAdvanced();  // if ANY are basic, use basic functions for movement

    std::string getReport();
    void loadJsonSettings(nlohmann::json axis_json);  // throws json::exception if there are problems

    // puts the value into the operational range of all motors on this axis
    TEKNIC_MOTOR_API_CODE putInRange(double& pos);

    // returns a number of enabled motors.
    int numEnabledMotors();

    // -------- settings for motors
    int getParallelGroup();
    void setParallelGroup(const int pg);
    void propagateConfig();

private:
    // 0 means not parallel with anything, 
    // 0 will be overwritten to allow multiple motors on axis if the axis is advanced
    int parallel_group = 0;
    Axis() = delete;
    Axis(Axis&) = delete;

    // added by addMotor and removed on destruction
    std::vector< std::unique_ptr<Motor> > motors;
};


class TeknicMotorApi {
public:
    TeknicMotorApi(const std::string motor_config_filename= "./configuration/motors.json",
        const std::string axes_config_filename = "./configuration/axes.json");
    ~TeknicMotorApi();

    // -------- main control functions TODO make sure returns are meaningful
    // Main move function of a list of axes to a position in the corresponding list.
    TEKNIC_MOTOR_API_CODE move(const std::vector<std::string> axes_names, 
        const std::vector<double> positions,
        const bool engageBrakes=true);

    // will move all axes to the neutral position
    // does not engage brakes after completion
    TEKNIC_MOTOR_API_CODE neutral_position();
    // will move axes to the neutral position
    // does not engage brakes after completion
    TEKNIC_MOTOR_API_CODE neutral_position(const std::vector<std::string> axes_names);

    // Homes all axes. if advanced axis, homes motors on them in parallel
    TEKNIC_MOTOR_API_CODE home();
    // Homes each axis. if advanced axis, homes motors on them in parallel
    TEKNIC_MOTOR_API_CODE home(const std::vector<std::string> axes_names);

    // Move of the config-based subset of axes:
    // will move only axes NOT MARKED as 'retreat_axis'
    // does not engage brakes after completion
    TEKNIC_MOTOR_API_CODE preshape(
        const std::vector<std::string> axes_names, 
        const std::vector<double> positions);
    // will move only axes MARKED as 'retreat_axis'
    // engages brakes after completion
    TEKNIC_MOTOR_API_CODE approach(
        const std::vector<std::string> axes_names, 
        const std::vector<double> positions);
    // will move all axes marked as 'retreat_axis' to the neutral position
    // does not engage brakes after completion
    TEKNIC_MOTOR_API_CODE retreat();

    // stop all axes and motors
    // thread-safe at the motor level
    void stop();

    // -------- accessory functions
    std::vector<TEKNIC_MOTOR_API_CODE> checkPosition(const std::vector<std::string> axes_names,
        std::vector<double>& positions);

    // -------- status
    // If false, delete the object and try again
    bool wasInitializedCorrectly();
    // were all motors on all axes homed?
    bool wereHomed();
    bool wereHomed(const std::vector<std::string> axes_names);
    // is the API currently engaged with moving or homing motors?
    bool isEngaged();

    // global timeout for waiting for action to complete, seconds
    double action_timeout = 300;

    /// Checks if the API code was an error (ret true) or OK or warning (ret false).
    /// 
    /// Errors usually mean that the function did not complete its primary purpose and any later
    /// code that depends on it should abort.
    static const bool isError(const TEKNIC_MOTOR_API_CODE& code);
    static const std::string codeMessage(const TEKNIC_MOTOR_API_CODE& code);

    // -------- names and reports
    std::vector<std::string> getAxesNames();
    std::vector<std::string> getAxesUnits();
    std::string getReport();

private:
    TeknicMotorApi() = delete;
    TeknicMotorApi(TeknicMotorApi&) = delete;

    bool initializedCorrectly = false;  // set by constructor
    // whether the API is busy doing smth. E.g. won't start a move if true.
    std::atomic<bool> engaged = false;  

    sFnd::SysManager* m_manager = nullptr;
    size_t portCount = 0;
    std::vector<std::set<int>> m_groups_in_ports;

    // is set to an unused group number to be able to trigger one axis separately during creation
    int homingGroupNumber = 999;  

    void engageBrakes();
    void disengageBrakes();

    void clearMoves();
    bool areAllMovesDone();
    bool isGroupMoveDone(const int group_number);

    std::vector< std::unique_ptr<Axis> > axes;
    int loadAxesConfiguration(const std::string axes_config_filename);

};

