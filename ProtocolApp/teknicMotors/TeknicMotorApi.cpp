//#include "pch.h"
#include "TeknicMotorApi.h"

using namespace sFnd;
using namespace std;
using json = nlohmann::json;


// AS maybe should export into utils
double round(const double val, const int num_numbers)
{
    if (num_numbers <= 0)
        return round(val);
    double factor = pow(10, num_numbers);
    return round(val * factor) / factor;
}

//------------------------------------------------------------------------------------
//--------------------------------------- MOTOR --------------------------------------
//------------------------------------------------------------------------------------
Motor::Motor(sFnd::INode& node, const json settings, const std::string setting_dir) :
    m_node(node) {

    disable();

    // parse the JSON variables
    string config_file = settings.at("config_file");
    int enabled = settings.at("enabled");
    disabled = !(bool)enabled;
    json conversion = settings.at("conversion");
    
    double input_offset = conversion.at("input_offset");
    double in_to_out_coefficient = conversion.at("in_to_out_coefficient");
    double vel_max = conversion.at("vel_max");
    double acc_max = conversion.at("acc_max");

    model = settings.at("model");
    timeout_for_enable = settings.value("timeout_for_enable", timeout_for_enable);
    basic_action_timeout = settings.value("basic_action_timeout", basic_action_timeout);

    // load settings onto motor
    m_node.Setup.ConfigLoad((setting_dir + "/" + config_file).c_str());

    // set up converters
    pos = Convertor(input_offset, in_to_out_coefficient, true);
    if (in_to_out_coefficient > 0) {
        vel = Convertor(0, vel_max / 100);  // input values 0-100, convertor is max/100
        acc = Convertor(0, acc_max / 100);
    }
    else {
        vel = Convertor(0, - vel_max / 100);  // input values 0-100, convertor is max/100
        acc = Convertor(0, - acc_max / 100);
    }
    vel.setInputRange(vector<double>{-vel_max, vel_max});
    vel.setInputRange(vector<double>{-acc_max, acc_max});

    // name
    name = m_node.Info.UserID.Value();

    // firmware has 1-7-D marking in the end, not sure what it means
    if ((string(m_node.Info.Model.Value())).find(model) == string::npos) {
        string buf = ("Motor " + name + " model differs between config (" + model +
            ") and ClearPath settings (" + m_node.Info.Model.Value() + ")." +
            " This can be very dangerous. Aborting initiation.");
        logError(buf.c_str());
        throw invalid_argument(buf);
    }

    // set units for velocity and acceleration
    m_node.VelUnit(INode::RPM);
    m_node.AccUnit(INode::RPM_PER_SEC);

    // set some of the parameters to be refreshed every time they are requested
    m_node.Motion.PosnMeasured.AutoRefresh(true);
    m_node.Motion.TrqMeasured.AutoRefresh(true);

    logInfo(getDetails().c_str());
    if (!disabled)
        prepareForAction();
}

Motor::~Motor(void) {
    stop();
    disable();
}

/// <summary>
/// Attempts to enable.
/// </summary>
/// <returns></returns>
TEKNIC_MOTOR_API_CODE Motor::enable() {
    auto startTimeoutTime = Times::getCurrentTime();
    string buf;

    // if it is enabled, no need to reenable
    if (motionIsReady())
        return TEKNIC_MOTOR_API_CODE::MOTOR_ALREADY_ENABLED;

    // Send enable request
    m_node.EnableReq(true);

    // define a timeout in case the node is unable to enable
    // This will loop checking on the Real time values of the node's Ready status
    while (!motionIsReady()) {
        if (Times::isTimeout(startTimeoutTime, timeout_for_enable)) {
            buf = "TeknicMotorDevice Error: Timed out waiting for Node " + name + " to enable.";
            logError(buf.c_str());
            return TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
        }
    }

    buf = (string("Motor ") + name + " enabled in " + 
        to_string(round(((double)Times::getElapsedMilliSecsSince(startTimeoutTime)) / 1000, 3)) + " seconds.");
    logInfo(buf.c_str());

    return TEKNIC_MOTOR_API_CODE::OK;
}

TEKNIC_MOTOR_API_CODE Motor::disable() {
    string buf;
    try
    {
        m_node.EnableReq(false);
        buf = "Disabled motor " + name + ".";
        logInfo(buf.c_str());
    }
    catch (mnErr& theErr) {
        buf = string("Motor disable error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR;
    }
    return TEKNIC_MOTOR_API_CODE::OK;
}

const bool Motor::isDisabledByConfig()
{
    return disabled;
}

void Motor::clearMoves()
{
    stop();
}

/// <summary>
/// Uses Tekinc homing routine.
/// 
/// TODO monitor errors or overheat or whatever we can - alarms, if anything in addition to
/// the default Teknic's catch of high torque or heat.
/// </summary>
/// <returns></returns>
TEKNIC_MOTOR_API_CODE Motor::homeBasic(const double& position_error_threshold) {
    if (TeknicMotorApi::isError(prepareForAction()))
        return TEKNIC_MOTOR_API_CODE::COULD_NOT_ENABLE_MOTOR;

    string buf;
    auto startTimeoutTime = Times::getCurrentTime();

    // check if homing has been set up
    if (!m_node.Motion.Homing.HomingValid()) {
        buf = "TeknicMotorDevice Error: Homing never setup through ClearView. Node " + name + " cannot be homed.";
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::MOTOR_HOMING_INVALID;
    }

    // report if node was homed before
    if (m_node.Motion.Homing.WasHomed()) {
        buf = "Motor " + name + " has already been homed, current position is:" + to_string(m_node.Motion.PosnMeasured.Value());
        logInfo(buf.c_str());
    }

    // initiate homing
    buf = "Homing motor " + name + " now...";
    logInfo(buf.c_str());
    try
    {
        m_node.Motion.Homing.Initiate();
    }
    catch (mnErr& theErr) {
        buf = string("Motor " + name + " homing error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR;
    }

    // define a timeout in case the node is unable to home
    while (isHoming()) {
        if (Times::isTimeout(startTimeoutTime, basic_action_timeout)) {
            buf = "Node " + name + " did not complete homing because of timeout.";
            logError(buf.c_str());
            stop();
            return TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
        }
    }

    // The motor has to have a correct flag and be in zero position
    if (m_node.Motion.Homing.WasHomed() && isInPosition(0, position_error_threshold)) {
        buf = "Motor " + name + " completed homing.";
        logInfo(buf.c_str());
    }
    else {
        buf = "Motor " + name + " has not completed homing.";
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::ACTION_COMPLETED_INCORRECTLY;
    }

    return TEKNIC_MOTOR_API_CODE::OK;
}

void Motor::setParallelGroup(const int pg)
{
    m_node.Motion.Adv.TriggerGroup(pg);
}

string Motor::getDetails() {
    std::string buf;

    switch (m_node.Info.NodeType()) {
    case IInfo::MERIDIAN_ISC:
        buf = "MERIDIAN_ISC";
        break;
    case IInfo::CLEARPATH_SC:
        buf = "CLEARPATH_SC";
        break;
    case IInfo::CLEARPATH_SC_ADV:
        buf = "CLEARPATH_SC_ADV";
        break;
    case IInfo::UNKNOWN:
    default:
        buf = "UNKNOWN";
        break;
    }

    buf = (
        "MotorInfo | NodeType: " + buf +
        ", Model: " + m_node.Info.Model.Value() +
        ", Serial #" + to_string(m_node.Info.SerialNumber.Value()) +
        ", FW version " + m_node.Info.FirmwareVersion.Value()) + ".";
    if (disabled)
        buf += " Disabled.";
    else
        buf += " Enabled.";
    return buf;
}

std::string Motor::getReport()
{
    std::string answ = name + " ";
    if (isAdvanced())
        answ += "advanced, ";
    else
        answ += "basic, ";
    if (disabled)
        answ += "disabled";
    else
        answ += "enabled";
    answ += ", " + model;
    return answ;
}

void Motor::setRange(const std::vector<double> v)
{
    pos.setInputRange(v);
}

double Motor::forceInRange(const double position)
{
    return pos.forceInRange(position);
}

std::vector<double> Motor::getPosRange()
{
    return pos.getInputRange();
}

void Motor::removeRange()
{
    pos.removeRange();
}

double Motor::getCurrentPosition()
{
    return pos.convertBack(m_getCurrentPosition());
}

void Motor::clearNodeStops()
{
    m_node.Motion.NodeStopClear();
}

/// <summary>
/// Clears and reports on alert buffer
/// 
/// TODO probably should handle and/or propagate on the found alerts
/// </summary>
void Motor::handleAlerts() {
    //This creates an Empty String for us to put our Alert State String into
    char alertList[256];
    string buf;  // for log errors and warnings

    // get a reference to our ValueAlert Object
    ValueAlert& myNodesAlert = m_node.Status.Alerts;

    // Refresh the Real-Time Status, and Alert Register
    m_node.Status.RT.Refresh();
    myNodesAlert.Refresh();

    // get an alertReg reference, and check directly for alerts
    if (myNodesAlert.Value().isInAlert())
    {
        // Put a text description of all alert bits set into our empty string, then print the string
        myNodesAlert.Value().StateStr(alertList, 256);
        buf = string("Node ") + name + " has alerts! Alerts: " + alertList;
        logWarning(buf.c_str());

        //// Check for specific alerts by accessing the cpmAlertFlds within the alertReg Struct
        // if (myNodesAlert.Value().cpm.TrackingShutdown)
        //{
        //    printf("Tracking Shutdown\n");
        //}
    }
    // _cpmAlertFlds for a complete list of Possible Alerts

    //Check to see if the node experienced torque saturation
    if (m_node.Status.HadTorqueSaturation()) {
        buf = "Node ";
        buf += name + " has experienced torque saturation since last checking";
        logWarning(buf.c_str());
    }

    m_node.Status.AlertsClear();
}

bool Motor::isMoveDone()
{
    INode::UseMutex myLock(m_node);
    return m_node.Motion.MoveIsDone();
}

bool Motor::wasHomed()
{
    if (isAdvanced()) {  // advanced supports homing wcustom homing routine
        if (m_node.Motion.Homing.WasHomed())
            return true;
    }
    else {  // basic has to have homing set up
        if (m_node.Motion.Homing.HomingValid()) {
            if (m_node.Motion.Homing.WasHomed())
                return true;
        }
        else {
            string buf = ("TeknicMotorApi Error: Homing never setup through ClearView for a basic motor " + name + "." +
                " Should not be asking if homed.");
            logWarning(buf.c_str());
        }
    }
    return false;
}

bool Motor::isHoming()
{
    INode::UseMutex myLock(m_node);
    return m_node.Motion.Homing.IsHoming();
}

bool Motor::isAdvanced()
{
    return m_node.Info.NodeType() == IInfo::CLEARPATH_SC_ADV;
}

bool Motor::isInPosition(const double& position, const double& position_error_threshold)
{
    return m_isInPosition((int) pos(position), (int) fabs(pos.normalize(position_error_threshold)));
}

double Motor::getTorque()
{
    INode::UseMutex myLock(m_node);
    return m_node.Motion.TrqMeasured.Value();
}

/// <summary>
/// Initiates move and waits for the move execution.
/// Uses basic features of the motor firmware.
/// In units of MOTOR - counts, RPM, RPM/s.
/// </summary>
TEKNIC_MOTOR_API_CODE Motor::m_moveBasic(const int& moveCounts, const double& velocity, const double& acceleration,
    const int& position_error_threshold_counts) {
    auto startTimeoutTime = Times::getCurrentTime();
    string buf;

    // enables and handles alerts
    if (TeknicMotorApi::isError(prepareForAction()))
        return TEKNIC_MOTOR_API_CODE::COULD_NOT_ENABLE_MOTOR;

    // Then set the velocity/accel:
    m_node.Motion.VelLimit = abs(velocity);
    m_node.Motion.AccLimit = abs(acceleration);

    // log
    int relativeMoveCounts = moveCounts - m_getCurrentPosition();
    buf = "Moving basic motor: " + name + " moveCounts: " + to_string(moveCounts) + " relative: " + to_string(relativeMoveCounts) + ".";
    logInfo(buf.c_str());

    // start the movement, does not block the thread
    try {
        m_node.Motion.MovePosnStart(moveCounts, true);
    }
    catch (mnErr& theErr) {
        buf = string("move Node error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR;
    }

    // lock the execution until the move is done or a timeout occured
    auto answ = TEKNIC_MOTOR_API_CODE::OK;
    while (!isMoveDone()) {
        if (Times::isTimeout(startTimeoutTime, basic_action_timeout)) {
            logError("Timed out waiting for move to complete.");
            stop();  // interrupts the movement and makes moveisdone true
            answ = TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
        }
    }

    // if end position is too far from the target
    if (!TeknicMotorApi::isError(answ) && !m_isInPosition(moveCounts, position_error_threshold_counts))
        answ = TEKNIC_MOTOR_API_CODE::ACTION_COMPLETED_INCORRECTLY;

    buf = ("Move complete on " + name + " in " + to_string(((double)Times::getElapsedMilliSecsSince(startTimeoutTime))/1000.) + " sec." + 
        " Current postion " + to_string(m_getCurrentPosition()) + " counts." +
        " Completion code " + to_string((int)answ));
    logInfo(buf.c_str());

    return answ;
}

/// <summary>
/// Sets speed and accelearion limits, etc.
/// Uses basic features of the motor firmware.
/// In units of MOTOR - counts, RPM, RPM/s.
/// </summary>
/// <param name="velocity"></param>
/// <param name="acceleration"></param>
/// <returns></returns>
TEKNIC_MOTOR_API_CODE Motor::m_prepareMoveBasic(const double velocity, const double acceleration)
{
    // enables and handles alerts
    if (TeknicMotorApi::isError(prepareForAction()))
        return TEKNIC_MOTOR_API_CODE::COULD_NOT_ENABLE_MOTOR;

    string buf;

    // Then set the velocity/accel:
    m_node.Motion.VelLimit = abs(velocity);
    m_node.Motion.AccLimit = abs(acceleration);

    // log
    buf = "Preparing to basic move motor " + name + ".";
    logInfo(buf.c_str());

    return TEKNIC_MOTOR_API_CODE::OK;
}


/// <summary>
/// Starts the movement and returns without waiting for execution.
/// Uses basic features of the motor firmware.
/// It should be enabled.
/// In units of MOTOR - counts, RPM, RPM/s.
/// </summary>
/// <param name="moveCounts"></param>
/// <returns></returns>
TEKNIC_MOTOR_API_CODE Motor::m_startMoveBasic(const int moveCounts)
{
    string buf;
    int relativeMoveCounts = moveCounts - m_getCurrentPosition();

    // log
    buf = ("Starting basic motor " + name + "."
        " Current postion " + to_string(m_getCurrentPosition()) + " counts."
        " Target position " + to_string(moveCounts) + " counts."
        " Relative " + to_string(moveCounts - m_getCurrentPosition()) + " counts.");
    logInfo(buf.c_str());

    // start the movement
    try {
        m_node.Motion.MovePosnStart(moveCounts, true);
    }
    catch (mnErr& theErr) {
        buf = string("Motion.MovePosnStart error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR;
    }

    return TEKNIC_MOTOR_API_CODE::OK;
}

/// <summary>
/// Sets a triggered absolute movement to ready. 
/// In units of MOTOR - counts, RPM, RPM/s.
/// The movement will be executed when a trigger command is sent to the motor's group
/// on the port.
/// </summary>
/// <param name="moveCounts"></param>
/// <param name="velocity"></param>
/// <param name="acceleration"></param>
/// <returns></returns>
TEKNIC_MOTOR_API_CODE Motor::m_prepareMoveAdvanced(const int moveCounts, const double velocity, const double acceleration)
{
    // enables and handles alerts
    if (TeknicMotorApi::isError(prepareForAction()))
        return TEKNIC_MOTOR_API_CODE::COULD_NOT_ENABLE_MOTOR;

    string buf;

    // Then set the velocity/accel:
    m_node.Motion.VelLimit = abs(velocity);
    m_node.Motion.AccLimit = abs(acceleration);

    // log
    buf = ("Preparing trigger to move advanced Node " + name + "."
        " Current postion " + to_string(m_getCurrentPosition()) + " counts."
        " Target position " + to_string(moveCounts) + " counts."
        " Relative " + to_string(moveCounts - m_getCurrentPosition()) + " counts.");
    logInfo(buf.c_str());

    // set the move
    try {
        m_node.Motion.Adv.MovePosnStart(moveCounts, true, true);  // absolute triggered
    }
    catch (mnErr& theErr) {
        buf = string("Motion.Adv.MovePosnStart error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR;
    }

    return TEKNIC_MOTOR_API_CODE::OK;
}

TEKNIC_MOTOR_API_CODE Motor::m_prepareHomeAdvanced(const double velocity, const double acceleration)
{
    // enables and handles alerts
    if (TeknicMotorApi::isError(prepareForAction()))
        return TEKNIC_MOTOR_API_CODE::COULD_NOT_ENABLE_MOTOR;

    string buf;

    // Then set the velocity/accel:
    m_node.Motion.VelLimit = abs(velocity);
    m_node.Motion.AccLimit = abs(acceleration);

    buf = ("Preparing trigger to home advanced motor " + name + ".");
    logInfo(buf.c_str());

    // set the move, sign indicates direction
    try {
        m_node.Motion.Adv.MoveVelStart(velocity, true);  // triggered
    }
    catch (mnErr& theErr) {
        buf = string("Motion.Adv.MoveVelStart error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR;
    }

    return TEKNIC_MOTOR_API_CODE::OK;
}

bool Motor::m_isInPosition(const int targetCounts, const int position_error_threshold_counts)
{
    // if end position is too far from the target
    if (abs(m_getCurrentPosition() - targetCounts) <= position_error_threshold_counts)
        return true;
    return false;
}

int Motor::m_getCurrentPosition()
{
    return (int)round(m_node.Motion.PosnMeasured.Value());
}

bool Motor::motionIsReady()
{
    INode::UseMutex myLock(m_node);
    return m_node.Motion.IsReady();
}

TEKNIC_MOTOR_API_CODE Motor::moveBasic(const double& position, const double& velocity, const double& acceleration, 
    const double& position_error_threshold)
{
    return m_moveBasic((int)pos(position), vel(velocity), acc(acceleration), (int)pos.normalize(position_error_threshold));
}

TEKNIC_MOTOR_API_CODE Motor::prepareMoveBasic(const double& velocity, const double& acceleration)
{
    return m_prepareMoveBasic(vel(velocity), acc(acceleration));
}

TEKNIC_MOTOR_API_CODE Motor::startMoveBasic(const double& position)
{
    return m_startMoveBasic((int)pos(position));
}

TEKNIC_MOTOR_API_CODE Motor::prepareMoveAdvanced(const double& position, const double& velocity, const double& acceleration)
{
    //string buf = "Preparing move with position " + to_string((int)pos(position)) + " velocity " + to_string(vel(velocity)) + " acc " + to_string(acc(acceleration)) + ".";
    //logInfo(buf.c_str());
    return m_prepareMoveAdvanced((int)pos(position), vel(velocity), acc(acceleration));
}

TEKNIC_MOTOR_API_CODE Motor::prepareHomeAdvanced(const double& velocity, const double& acceleration)
{
    //string buf = "Preparing home with velocity " + to_string(vel(velocity)) + ".";
    //logInfo(buf.c_str());
    return m_prepareHomeAdvanced(vel(velocity), acc(acceleration));
}

void Motor::zeroMotor()
{
    string buf = "Motor " + name + " position sensor has been set to zero.";
    logInfo(buf.c_str());
    m_node.Motion.AddToPosition(-(double)m_node.Motion.PosnMeasured.Value());
}

void Motor::markMotorHomed()
{
    string buf = "Motor " + name + " is marked as homed.";
    m_node.Motion.Homing.SignalComplete();
}

void Motor::stop()
{
    INode::UseMutex myLock(m_node);
    m_node.Motion.NodeStop(STOP_TYPE_ABRUPT);
    // m_node.Motion.NodeStop(STOP_TYPE_ESTOP_ABRUPT);  // <-- this one will require flushing the error buf
}

TEKNIC_MOTOR_API_CODE Motor::prepareForAction()
{
    // clears alert buffer
    handleAlerts();
    // allows movement if a ESTOP stop was used before
    clearNodeStops();
    // attempt enable
    return enable();
}

TEKNIC_MOTOR_API_CODE Motor::triggerSameGroupMovement()
{
    string buf = "Triggering advanced group " + to_string(m_node.Motion.Adv.TriggerGroup()) + " move.";
    logInfo(buf.c_str());

    try {
        m_node.Motion.Adv.TriggerMovesInMyGroup();
    }
    catch (mnErr& theErr) {
        buf = string("Motion.Adv.TriggerMovesInMyGroup error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR;
    }

    return TEKNIC_MOTOR_API_CODE::OK;
}


//------------------------------------------------------------------------------------
//--------------------------------------- AXIS ---------------------------------------
//------------------------------------------------------------------------------------

Axis::Axis(json axis_json)
{
    loadJsonSettings(axis_json);
}

Axis::~Axis()
{
    motors.clear();
}

TEKNIC_MOTOR_API_CODE Axis::addMotor(sFnd::INode& node, nlohmann::json motor_json, const std::string setting_dir)
{
    string buf;
    try
    {
        motors.push_back(make_unique<Motor>(node, motor_json, setting_dir));
    }
    catch (const mnErr& theErr)
    {
        buf = (string("Teknic error during motor addition. Error message: ") + theErr.ErrorMsg);
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR;
    }
    catch (const json::exception& e)
    {
        buf = (string("JSON parsing error during motor addition. Error message: ") + e.what());
        logError(buf.c_str());
        return TEKNIC_MOTOR_API_CODE::JSON_PARSING_ERROR;
    }
    return TEKNIC_MOTOR_API_CODE::OK;
}

TEKNIC_MOTOR_API_CODE Axis::homeBasic()
{
    auto startTimeoutTime = Times::getCurrentTime();
    string buf;

    // checks
    if (numEnabledMotors() == 0)
        return TEKNIC_MOTOR_API_CODE::NO_MOTORS_ON_AXIS;
    if (areAllAdvanced()) {
        buf = "Axis " + name + " is advanced, it is recommended to use advanced move functions instead.";
        logWarning(buf.c_str());
    }

    // flags for tasks completions
    TEKNIC_MOTOR_API_CODE res = TEKNIC_MOTOR_API_CODE::OK;

    // for each associated motor, run the basic homing routine
    for (auto& motor : motors) {
        // skip config-disabled motors
        if (motor->isDisabledByConfig())
            continue;

        // home
        res = motor->homeBasic(position_error_threshold);

        // check for errors
        if (TeknicMotorApi::isError(res)) {
            stop();
            buf = "Axis " + name + " encountered an error during basic homing. Aborting homing of this axis.";
            logError(buf.c_str());
            return res;
        }

        // check timeout
        if (Times::isTimeout(startTimeoutTime, homing_timeout)) {
            stop();  // interrupts the movement and makes moveisdone true
            buf = "Axis " + name + " Timed out waiting for homing to complete.";
            logError(buf.c_str());
            return TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
        }
    }

    return res;
}

// TODO monitor errors or overheat or whatever we can - alarms
TEKNIC_MOTOR_API_CODE Axis::moveBasic(const double& position, const double& velocity, const double& acceleration)
{
    // for advanced axes use group-based move instead
    auto startTimeoutTime = Times::getCurrentTime();
    string buf;

    // checks
    if (numEnabledMotors() == 0)
        return TEKNIC_MOTOR_API_CODE::NO_MOTORS_ON_AXIS;
    if (areAllAdvanced()) {
        buf = "Axis " + name + " is advanced, it is recommended to use advanced move functions instead.";
        logWarning(buf.c_str());
    }

    // flags for tasks completions
    TEKNIC_MOTOR_API_CODE res = TEKNIC_MOTOR_API_CODE::OK;

    // for each associated motor, prepare the move
    for (auto& motor : motors) {
        // skip config-disabled motors
        if (motor->isDisabledByConfig())
            continue;

        // prepare the move
        res = motor->prepareMoveBasic(velocity, acceleration);

        // check for errors
        if (TeknicMotorApi::isError(res)) {
            clearMoves();
            buf = "Axis " + name + " encountered an error during basic moving preparation. Aborting moving of this axis.";
            logError(buf.c_str());
            return res;
        }
    }

    // send the go signal
    for (auto& motor : motors) {
        // skip config-disabled motors
        if (motor->isDisabledByConfig())
            continue;

        // start the move
        res = motor->startMoveBasic(position);

        // check for errors
        if (TeknicMotorApi::isError(res)) {
            stop();
            buf = "Axis " + name + " encountered an error during basic moving start. Aborting moving of this axis.";
            logError(buf.c_str());
            return res;
        }
    }

    // monitor the end or timeout
    bool moveIsDone = false;
    while (!moveIsDone) {
        if (Times::isTimeout(startTimeoutTime, homing_timeout)) {
            stop();  // interrupts the movement and makes moveisdone true
            buf = "Axis " + name + " timed out waiting for basic move to complete.";
            logError(buf.c_str());
            return TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
        }

        moveIsDone = isAllMoveDone();
    }

    // check the final position
    for (auto& motor : motors) {
        if (!motor->isDisabledByConfig() && !motor->isInPosition(position, position_error_threshold)) {
            buf = "Axis " + name + " failed to get in position after move. Current position ";
            buf += to_string(motor->getCurrentPosition()) + ", target: " + to_string(position) + 
                ", threshold: " + to_string(position_error_threshold) + ".";
            logError(buf.c_str());
            res = TEKNIC_MOTOR_API_CODE::ACTION_COMPLETED_INCORRECTLY;
            break;
        }
    }
    if (TeknicMotorApi::isError(res)) {
        buf = "Axis " + name + " failed to complete basic movement.";
        logError(buf.c_str());
    }
    else {
        buf = "Axis " + name + " completed basic move successfully.";
        logInfo(buf.c_str());
    }

    return res;
}

TEKNIC_MOTOR_API_CODE Axis::moveBasic(const double& position)
{
    return moveBasic(position, default_vel, default_acc);
}

TEKNIC_MOTOR_API_CODE Axis::prepareAdvancedMovement(const double& position, const double& velocity, const double& acceleration)
{
    // checks
    if (numEnabledMotors() == 0)
        return TEKNIC_MOTOR_API_CODE::NO_MOTORS_ON_AXIS;
    if (!areAllAdvanced())
        return TEKNIC_MOTOR_API_CODE::DOES_NOT_SUPPORT_FUNCTION;

    TEKNIC_MOTOR_API_CODE answ = TEKNIC_MOTOR_API_CODE::OK;
    for (auto& motor : motors) {
        // skip config-disabled motors
        if (motor->isDisabledByConfig())
            continue;

        answ = motor->prepareMoveAdvanced(position, velocity, acceleration);
        if (TeknicMotorApi::isError(answ)) {
            clearMoves();  // this will clear any moves that were already scheduled
            break;  // wrapping movement should not continue execution
        }
    }

    return answ;
}

TEKNIC_MOTOR_API_CODE Axis::prepareAdvancedMovement(const double& position)
{
    return prepareAdvancedMovement(position, default_vel, default_acc);
}

TEKNIC_MOTOR_API_CODE Axis::homeAdvanced(int freeGroupNumber)
{
    // checks
    if (numEnabledMotors() == 0)
        return TEKNIC_MOTOR_API_CODE::NO_MOTORS_ON_AXIS;
    if (!areAllAdvanced())
        return TEKNIC_MOTOR_API_CODE::DOES_NOT_SUPPORT_FUNCTION;

    // clear any moves
    clearMoves();

    auto startTimeoutTime = Times::getCurrentTime();
    string buf;

    // set the available group number on all associated motors
    int used_group_number = parallel_group;
    setParallelGroup(freeGroupNumber);

    // prepare triggered velocity move on all motors
    TEKNIC_MOTOR_API_CODE res = TEKNIC_MOTOR_API_CODE::OK;
    for (auto& motor : motors) {
        // skip config-disabled motors
        if (motor->isDisabledByConfig())
            continue;

        res = motor->prepareHomeAdvanced(default_vel * homing_direction, default_acc);

        // check for errors
        if (TeknicMotorApi::isError(res)) {
            stop();
            buf = "Axis " + name + " encountered an error during advanced homing. Aborting.";
            logError(buf.c_str());
            return res;
        }
    }

    // trigger movement with same group numbers on the first enabled motor
    for (auto& motor : motors)
        if (!motor->isDisabledByConfig()) {
            res = motor->triggerSameGroupMovement();
            break;
        }
    // check for errors
    if (TeknicMotorApi::isError(res)) {
        stop();
        buf = "Axis " + name + " encountered an error during triggering same group move. Aborting homing.";
        logError(buf.c_str());
        return res;
    }

    //  monitor the torque for it to cross threshold or timeout
    bool ordinary_exit = false;
    auto torqueReachedPeriod = Times::getElapsedMicroSecsSince(startTimeoutTime);
    double torque = 0;
    while (!ordinary_exit) {
        // check timeout
        if (Times::isTimeout(startTimeoutTime, homing_timeout)) {
            stop();
            buf = "Axis " + name + " timed out waiting for homing to complete.";
            logError(buf.c_str());
            return TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
        }

        // check someone pressing "stop"
        if (isAllMoveDone()) {
            return TEKNIC_MOTOR_API_CODE::USER_INTERRUPT;
        }

        // check motors excedeing torque max
        switch (homing_stop_condition)
        {
        case 0:
            // ANY motor reached torque, everyone stops
            for (auto& motor : motors) {
                if (motor->isDisabledByConfig())
                    continue;
                torque = abs(motor->getTorque());
                if (torque >= homing_torque_threshold_percent) {
                    stop();
                    torqueReachedPeriod = Times::getElapsedMicroSecsSince(startTimeoutTime);
                    ordinary_exit = true;  // otherwise isAllMoveDone triggers error break
                    break;
                }
            }
            break;
        case 1:
            // ALL: when each motor reaches torque, it stops. Homing is done when all motors stop.
            for (auto& motor : motors) {
                if (motor->isDisabledByConfig() || motor->isMoveDone())
                    continue;
                torque = abs(motor->getTorque());
                if (torque >= homing_torque_threshold_percent) {
                    motor->stop();
                }
            }
            if (isAllMoveDone()) {  // all motors stopped
                torqueReachedPeriod = Times::getElapsedMicroSecsSince(startTimeoutTime);
                ordinary_exit = true;  // otherwise isAllMoveDone triggers error break
            }
            break;
        default:
            stop();
            return TEKNIC_MOTOR_API_CODE::MOTOR_HOMING_INVALID;
            break;
        }
    }

    buf = "Homing " + name + " reached torque " + to_string(torque) + " in " + to_string(torqueReachedPeriod) + " usec.";
    logInfo(buf.c_str());

    // remove range constraints
    for (auto& motor : motors) {
        if (motor->isDisabledByConfig())
            continue;
        motor->removeRange();
    }

    // set up a move away from the hard stop
    for (auto& motor : motors) {
        if (motor->isDisabledByConfig())
            continue;
        motor->zeroMotor();
    }
    double postStopMove = homing_offset_in_units * homing_direction * -1;
    for (auto& motor : motors) {
        if (motor->isDisabledByConfig())
            continue;

        res = motor->prepareMoveAdvanced(postStopMove, default_vel, default_acc);
        // check for errors
        if (TeknicMotorApi::isError(res)) {
            stop();
            buf = "Axis " + name + " encountered an error during preparation of advanced move from hard stop. Aborting homing.";
            logError(buf.c_str());
            return res;
        }
    }

    // trigger movement
    for (auto& motor : motors)
        if (!motor->isDisabledByConfig()) {
            res = motor->triggerSameGroupMovement();
            break;
        }
    // check for errors
    if (TeknicMotorApi::isError(res)) {
        stop();
        buf = "Axis " + name + " encountered an error during triggering same group move. Aborting homing.";
        logError(buf.c_str());
        return res;
    }

    // monitor the move back
    while (!isAllMoveDone()) {
        if (Times::isTimeout(startTimeoutTime, homing_timeout)) {
            stop();  // interrupts the movement and makes moveisdone true
            buf = "Axis " + name + " timed out waiting for homing to complete.";
            logError(buf.c_str());
            return TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
        }
    }

    // check if final position is correct
    for (auto& motor : motors) {
        if (motor->isDisabledByConfig())
            continue;

        if (!motor->isInPosition(postStopMove, position_error_threshold)) {
            buf = "Axis " + name + " failed to get in position after hard stop during homing. ";
            buf += "Current position " + to_string(motor->getCurrentPosition()) + ", ";
            buf += "target: " + to_string(postStopMove) + ", ";
            buf += "threshold: " + to_string(position_error_threshold) + ".";
            logError(buf.c_str());
            return TEKNIC_MOTOR_API_CODE::ACTION_COMPLETED_INCORRECTLY;
        }
    }

    // add range constraints back
    for (auto& motor : motors) {
        if (motor->isDisabledByConfig())
            continue;

        motor->setRange(range);
    }

    // mark the homing as completed
    for (auto& motor : motors) {
        motor->zeroMotor();
        motor->markMotorHomed();
    }

    // return the original group number to all motors
    setParallelGroup(used_group_number);

    return TEKNIC_MOTOR_API_CODE::OK;
}

void Axis::stop()
{
    for (auto& motor : motors) {
        motor->stop();
    }
}

void Axis::disable()
{
    for (auto& motor : motors)
        if (!motor->isDisabledByConfig())
            motor->disable();
}

void Axis::clearMoves()
{
    for (auto& motor : motors)
        motor->clearMoves();
}

bool Axis::wereAllHomed()
{
    bool answ = true;
    for (auto& motor : motors)
        if (!motor->isDisabledByConfig())
            answ = answ && motor->wasHomed();
    return answ;
}

bool Axis::isAllMoveDone()
{
    bool moveIsDone = true;
    for (auto& motor : motors)
        if (!motor->isDisabledByConfig())
            moveIsDone = moveIsDone && motor->isMoveDone();
    return moveIsDone;
}

bool Axis::areAnyHoming()
{
    bool is_homing = false;
    for (auto& motor : motors)
        if (!motor->isDisabledByConfig())
            is_homing = is_homing || motor->isHoming();
    return is_homing;
}

bool Axis::areAllAdvanced()
{
    bool advanced = true;
    for (auto& motor : motors)
        if (!motor->isDisabledByConfig())
            advanced = advanced && motor->isAdvanced();
    return advanced;
}

std::string Axis::getReport()
{
    string answ = name + ", ";
    if (areAllAdvanced())
        answ += "advanced";
    else
        answ += "basic";
    answ += ", group " + to_string(getParallelGroup()) + ", ";
    if (retreat_axis)
        answ += "retreat";
    else
        answ += "preshape";
    answ += " axis.";
    if (range.size() > 1)
        answ += " Range: [" + to_string(range[0]) + ", " + to_string(range[1]) + "].";
    answ += "\r\n";
    for (auto& motor : motors) {
        answ += "\t" + motor->getReport() + "\r\n";
    }

    return answ;
}

void Axis::loadJsonSettings(json axis_json)
{
    name = axis_json.at("name");
    retreat_axis = (bool) (int) axis_json.at("retreat_axis");
    units = axis_json.at("units");

    default_vel = axis_json.at("default_vel");
    default_acc = axis_json.at("default_acc");
    neutral_pos = axis_json.at("neutral_position");

    // no reason to call the setParallelGroup as this is executed before motors are attached
    parallel_group = axis_json.at("parallel_group");

    json homing = axis_json.at("homing");
    homing_direction = homing.at("direction");
    if (homing_direction > 0)  // restrict to be +-1
        homing_direction = 1;
    else
        homing_direction = -1;
    homing_offset_in_units = homing.at("offset_in_units");
    homing_offset_in_units = abs(homing_offset_in_units);  // always in the opposite direction bc of hardstop
    homing_torque_threshold_percent = homing.at("torque_threshold_percent");
    homing_timeout = homing.at("timeout");
    string s_stop_condition = homing.value("stop_condition", "any");
    if (!s_stop_condition.compare("any"))
        homing_stop_condition = 0;
    else if (!s_stop_condition.compare("all"))
        homing_stop_condition = 1;
    else
        throw invalid_argument("Received " + s_stop_condition + " as stop_condition in axis config.");

    range = { axis_json.at("pos_min"), axis_json.at("pos_max") };
    position_error_threshold = axis_json.value("position_error_threshold", abs(range[1] - range[0]) * 0.01);

    basic_action_timeout = axis_json.value("basic_action_timeout", basic_action_timeout);
}

TEKNIC_MOTOR_API_CODE Axis::putInRange(double& pos)
{
    if (numEnabledMotors() == 0)
        return TEKNIC_MOTOR_API_CODE::NO_MOTORS_ON_AXIS;  // warning - no motors

    // get minimums and maximums from all motors
    int fMotor = 0;
    for (; fMotor < motors.size(); fMotor++)
        if (!motors[fMotor]->isDisabledByConfig())
            break;
    vector<double> range = motors[fMotor]->getPosRange();
    double axis_min = range[0];
    double axis_max = range[1];
    for (int i_motor = fMotor+1; i_motor < motors.size(); i_motor++)
    {
        if (motors[i_motor]->isDisabledByConfig())
            continue;

        range = motors[i_motor]->getPosRange();
        if (range[0] > axis_min)
            axis_min = range[0];
        if (range[1] < axis_max)
            axis_max = range[1];
    }
    // check if they make sense
    if (axis_min > axis_max) {
        logError("Motor ranges for axes are not consistent. Minimum is higher than maximum.");
        return TEKNIC_MOTOR_API_CODE::RANGE_INCONSISTENT;
    }

    // bind
    if (pos < axis_min)
        pos = axis_min;
    else if (pos > axis_max)
        pos = axis_max;

    return TEKNIC_MOTOR_API_CODE::OK;
}

int Axis::numEnabledMotors()
{
    int answ = 0;
    for (auto& motor : motors)
        if (!motor->isDisabledByConfig())
            answ++;
    return answ;
}

int Axis::getParallelGroup()
{
    return parallel_group;
}

void Axis::setParallelGroup(const int pg)
{
    parallel_group = pg;
    for (auto& motor : motors) {
        if (!motor->isDisabledByConfig())
            motor->setParallelGroup(pg);
    }
    string buf;
    buf = "Set parallel group " + to_string(pg) + " for axis " + name + ".";
    logInfo(buf.c_str());
}

// can be used to send any other configuration settings to motors
void Axis::propagateConfig()
{
    for (auto& motor : motors) {
        if (!motor->isDisabledByConfig())
            motor->setRange(range);
    }
}


//------------------------------------------------------------------------------------
//--------------------------------------- MotorAPI -----------------------------------
//------------------------------------------------------------------------------------

/*
 *
 * Figure out how many SC4-HUBs are daisy chained ([0-2] per port), and how many
 * servo motors (nodes) are plugged into them. Register them to the SysManager.
 *
 * Ports are literal COM serial ports, COM6 by default on ours.
 * 
 */

/// <summary>
/// Attempts to intialize motors and set their parameters. If successfull, isInitializedCorrectly() will return True.
/// 
/// Sequence: 
///     get Manager instance;
///     get hub ports;
///     load JSON axis config;
///     load JSON motor config;
///     open ports;
///     go through all ports and motors, populating axes vector;
///     process and reassign parallel group numbers.
/// If at any step an error occurs, the initialization quits and isInitializedCorrectly() will return False.
/// </summary>
/// <param name="motor_config_filename"></param>
/// <param name="axes_config_filename"></param>
TeknicMotorApi::TeknicMotorApi(const std::string motor_config_filename, const std::string axes_config_filename) {
    // this flag is set to true at the end of constructor. 
    // If the constructor did not make it there, API behavior is upredictable
    initializedCorrectly = false;

    // buffer for error and warning reports
    string buf;

    // get manager instance and find ports
    std::vector<std::string> comHubPorts;
    try {
        m_manager = SysManager::Instance();
        m_manager->FindComHubPorts(comHubPorts);
    }
    catch (mnErr& theErr) {
        buf = (string("TeknicMotorApi() constructor Sys manager and FindComPorts error | addr: ") + to_string(theErr.TheAddr) +
            " | err: " + to_string(theErr.ErrorCode) + " | msg: " + theErr.ErrorMsg);
        logError(buf.c_str());
    }

    // check if com ports are present
    if (comHubPorts.empty()) {
        logError("TeknicMotorApi Error: No SC Hubs found! Motors not enabled.");
        return;
    }

    // Log the number of ports found
    portCount = comHubPorts.size();
    buf = "Found " + to_string(portCount) + " SC ComHubs";
    logInfo(buf.c_str());

    if (portCount < 1) {
        buf = "Not enough SC ComHubs ports. Aborting Motor setup.";
        logError(buf.c_str());
        return;
    }

    // load axes configuration file
    if (loadAxesConfiguration(axes_config_filename) < 0) {
        buf = "TeknicMotorApi. Error while loading axes configuration filename. Terminating motor initialization.";
        logError(buf.c_str());
        return;
    }

    // load motor configuration file
    string setting_dir = Folders::dirname(motor_config_filename);
    ifstream ifs(motor_config_filename);
    if (ifs.fail()) {
        buf = "TeknicMotorApi. Could not open motor settings JSON file: " + motor_config_filename + ". Check if it exists.";
        logError(buf.c_str());
        return;
    }
    json motors_settings;
    try
    {
        motors_settings = json::parse(ifs);
    }
    catch (const json::parse_error& e)
    {
        buf = "Parse error during motor settings JSON file read: " + string(e.what());
        logError(buf.c_str());
        return;
    }

    // set a local serial number id to the named com port
    for (size_t i = 0; i < portCount && i < NET_CONTROLLER_MAX; i++)
        m_manager->ComHubPort(i, comHubPorts[i].c_str());

    // open ports
    try {
        m_manager->PortsOpen(portCount);
    }
    catch (mnErr& theErr) {
        buf = ("MotorAPI() constructor PortsOpen | addr: " + to_string(theErr.TheAddr) +
            " | err: " + to_string(theErr.ErrorCode) + " | msg: " + theErr.ErrorMsg);
        logError(buf.c_str());
        return;
    }

    // Collect the parallel groups into port-specific sets
    m_groups_in_ports.clear();
    for (size_t i = 0; i < portCount; i++) {
        m_groups_in_ports.push_back(set<int>());
    }

    // Initialize nodes. Have to iterate ports, then nodes per port
    for (int i_port = 0; i_port < portCount; i_port++) { // For each port:
        buf = "Iterating nodes on port " + to_string(i_port) + ".";
        logInfo(buf.c_str());

        sFnd::IPort &port = m_manager->Ports(i_port);

        // Makes both (possible) breaks on the port allow motion
        port.BrakeControl.BrakeSetting(0, BRAKE_ALLOW_MOTION);
        port.BrakeControl.BrakeSetting(1, BRAKE_ALLOW_MOTION);

        // log
        buf = ("Port: " + to_string(port.NetNumber()) +
            ", State: " + to_string(port.OpenState()) +
            ", Node count: " + to_string(port.NodeCount()) + ".");
        logInfo(buf.c_str());

        // Iterate nodes on this port
        for (int nodeIndex = 0; nodeIndex < port.NodeCount(); nodeIndex++) {
            // get the correct setting if it exists
            // when a motor does not have a correct associated config, break initialization
            json single_mc;
            string motor_mot_filename;
            string axis_name;
            try 
            {
                single_mc = motors_settings.at(to_string(nodeIndex));
                motor_mot_filename = single_mc.at("config_file");
                axis_name = single_mc.at("axis");
            }
            catch (const json::exception& e) 
            {
                buf = "Error during motor settings JSON file read: " + string(e.what());
                logError(buf.c_str());
                return;
            }
            logInfo((string("Motor ") + to_string(nodeIndex) + ". Config file: " + motor_mot_filename
                + ". Axis: " + axis_name).c_str());

            // identify the correct axis
            int found_axis = -1;
            for (int i_axis = 0; i_axis < axes.size(); i_axis++)
            {
                if (axes[i_axis]->name == axis_name) {
                    found_axis = i_axis;
                    break;
                }
            }
            
            // if axis is incorrect, break the initialization
            if (found_axis == -1) {
                buf = "Could not find axis with the name " + axis_name + " in the list of axes.";
                logError(buf.c_str());
                return;
            }

            // otherwise add the motor to the node
            if ( isError(axes[found_axis]->addMotor(port.Nodes(nodeIndex), single_mc, setting_dir)) ) {
                buf = ("Error during Node " + to_string(nodeIndex) +
                    " initialization.");
                logError(buf.c_str());
                axes.clear();
                return;
            }
            axes[found_axis]->port = i_port;

            // from the found axis, add the parallel group to this port's set of groups
            m_groups_in_ports.back().insert(axes[found_axis]->getParallelGroup());
        }
    }

    // --------
    // process group numbers and reassign them to axes if there are conflicts in the numbers
    // --------

    // check if any Non-Advanced axes have non-zero groups, make them zero
    for (auto& axis : axes) {
        if (!axis->areAllAdvanced() && axis->getParallelGroup() != 0) {
            buf = "Setting basic axis " + axis->name + " to parallel group 0";
            logInfo(buf.c_str());
            axis->setParallelGroup(0);
        }
    }

    // go through all ports and see if any groups split between ports
    // if there are, break initialization
    // also, build a list of all group numbers
    // this routine also ensures that if retreat is split between different axes, it will be in different groups
    set<int> all_group_numbers = m_groups_in_ports[0];
    for (size_t i_port = 1; i_port < m_groups_in_ports.size(); i_port++)
    {
        // check if this port has any groups present in the previous ports
        for (int g : m_groups_in_ports[i_port]) {
            if (all_group_numbers.find(g) == all_group_numbers.end()) {  // does not have
                all_group_numbers.insert(g);
            }
            else {
                buf = ("MotorAxisApi Error. One parallel group is split between different ports,"
                    " which cannot be supproted programatically.. Please correct the config.Aborting initialization.");
                logError(buf.c_str());
                axes.clear();
                return;
            }
        }
    }

    buf = "Found " + to_string(all_group_numbers.size()) + " groups:";
    for (int gn : all_group_numbers)
        buf += " " + to_string(gn);
    buf += ".";
    logInfo(buf.c_str());

    int axis_group;
    int new_axis_group;

    // check if an advanced axis has multiple motors and a zero group, and change that axis to a new, non-zero group value
    // note: auto for loop does not allow iterating over the vector twice
    for (size_t i_axis = 0; i_axis < axes.size(); i_axis++) {
        // skip zero axes or non-retreat ones
        if (!axes[i_axis]->areAllAdvanced())
            continue;
        axis_group = axes[i_axis]->getParallelGroup();
        if (axis_group != 0) {
            continue;
        }

        // generate a new group number
        new_axis_group = *all_group_numbers.rbegin() + 1;  // largest element +1

        all_group_numbers.insert(new_axis_group);  // insert it into the list of all
        m_groups_in_ports[axes[i_axis]->port].insert(new_axis_group);  // add to the associated list
        axes[i_axis]->setParallelGroup(new_axis_group);
    }

    // check if any retreat axis shares a group with any other non-retreat axis, if so, pick a new parallel group# for it
    for (size_t i_axis = 0; i_axis < axes.size(); i_axis++) {
        // skip zero axes or non-retreat ones
        if (!axes[i_axis]->retreat_axis)
            continue;
        axis_group = axes[i_axis]->getParallelGroup();
        if (axis_group == 0) {  // non-advanced non-parallel axis
            continue;
        }

        buf = "Found retreat axis " + axes[i_axis]->name + " with group " + to_string(axis_group);
        logInfo(buf.c_str());

        new_axis_group = axis_group;
        // see if it overlaps with any
        for (auto& axis2 : axes) {
            if (axis2->getParallelGroup() == axis_group && !axis2->retreat_axis) {
                // generate a new group number
                new_axis_group = *all_group_numbers.rbegin() + 1;  // largest element +1
                break;
            }
        }

        if (axis_group != new_axis_group) {
            buf = "Replacing it with group number " + to_string(new_axis_group) + ".";
            logInfo(buf.c_str());
            all_group_numbers.insert(new_axis_group);  // insert it into the list of all
            m_groups_in_ports[axes[i_axis]->port].insert(new_axis_group);  // add to the associated list
            // go through each retreat axis with this group number and change its group number
            for (size_t i_axis2 = 0; i_axis2 < axes.size(); i_axis2++) {
                if (axes[i_axis2]->getParallelGroup() == axis_group && axes[i_axis2]->retreat_axis) {
                    axes[i_axis2]->setParallelGroup(new_axis_group);
                }
            }
        }
    }

    // set parallel group for each axis - propagate to motors
    // also propagate the ranges (config) to the motors
    // and generate a report if there are basic axes with multiple motors
    for (auto& axis : axes) {
        buf = "Axis " + axis->name + " has group " + to_string(axis->getParallelGroup()) + ".";
        logInfo(buf.c_str());
        axis->setParallelGroup(axis->getParallelGroup());
        if (!axis->areAllAdvanced() && axis->numEnabledMotors() > 1) {
            buf = ("Axis " + axis->name + " has multiple motors but is not advanced." +
                " Procede with moving this axis ONLY if the motors are NOT mechanically coupled.");
            logWarning(buf.c_str());
        }
        axis->propagateConfig();
    }

    homingGroupNumber = *all_group_numbers.rbegin() + 100;  // largest element +100
    buf = "Set homing group as " + to_string(homingGroupNumber) + ".";
    logInfo(buf.c_str());

    initializedCorrectly = true;
}

TeknicMotorApi::~TeknicMotorApi(void) {

    logInfo("Teknic shutting down.");

    axes.clear();
    m_manager->PortsClose();

    logInfo("Teknic Shutdown.");
}

TEKNIC_MOTOR_API_CODE TeknicMotorApi::home()
{
    return home(getAxesNames());
}

TEKNIC_MOTOR_API_CODE TeknicMotorApi::home(const std::vector<std::string> axes_names)
{
    if (!wasInitializedCorrectly())
        return TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR;

    if (isEngaged())
        return TEKNIC_MOTOR_API_CODE::ENGAGED;

    engaged = true;

    disengageBrakes();

    TEKNIC_MOTOR_API_CODE answ = TEKNIC_MOTOR_API_CODE::OK;
    string buf;
    bool found;
    auto startTime = Times::getCurrentTime();
    for (auto& axis : axes) {
        // check timeout
        if (Times::isTimeout(startTime, action_timeout)) {
            stop();  // this will mark all movements as done
            answ = TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
            break;
        }

        // is this axis present
        found = false;
        for (size_t i = 0; i < axes_names.size(); i++)
        {
            if (axis->name == axes_names[i]) {
                found = true;
                break;
            }
        }

        if (!found)
            continue;
            
        if (axis->areAllAdvanced())
            answ = axis->homeAdvanced(homingGroupNumber);
        else
            answ = axis->homeBasic();
        
        if (isError(answ))
        {
            buf = "Error " + to_string((int)answ) + " during homing of axis " + axis->name + ".";
            logError(buf.c_str());
            break;
        }
    }

    // check if was correctly homed but no other errors reported
    if (!isError(answ) && !wereHomed(axes_names))
        answ = TEKNIC_MOTOR_API_CODE::ACTION_COMPLETED_INCORRECTLY;

    engaged = false;

    return answ;
}

TEKNIC_MOTOR_API_CODE TeknicMotorApi::neutral_position()
{
    return neutral_position(getAxesNames());
}

TEKNIC_MOTOR_API_CODE TeknicMotorApi::neutral_position(const std::vector<std::string> axes_names)
{
    if (!wasInitializedCorrectly())
        return TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR;

    // make a list of axes and position to move
    vector<string> np_axes_names;
    vector<double> np_positions;

    // identify all axes' neutral positons
    // maintain order of the axes_names
    for (size_t i_axis = 0; i_axis < axes_names.size(); i_axis++)
    {
        for (auto& axis : axes) {
            if (axis->name == axes_names[i_axis]) {
                // found the axis with correct name
                np_axes_names.push_back(axis->name);
                np_positions.push_back(axis->neutral_pos);
                break;
            }
        }
    }

    return move(np_axes_names, np_positions, false);  // don't engage brakes after neutral position move
}

TEKNIC_MOTOR_API_CODE TeknicMotorApi::retreat()
{
    if (!wasInitializedCorrectly())
        return TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR;

    // make a list of axes and position to move
    vector<string> retreat_axes_names;
    vector<double> retreat_positions;

    for (auto& axis : axes) {
        if (axis->retreat_axis) {
            retreat_axes_names.push_back(axis->name);
            retreat_positions.push_back(axis->neutral_pos);
        }
    }

    return move(retreat_axes_names, retreat_positions, false);  // don't engage brakes after retreat
}

/// <summary>
/// Main move function
/// 
/// Operations:
///     disengage brakes;
///     clear moves;
///     move basic axes and set triggers for advanced axes;
///     if that was successful, trigger advanced axes group by group;
///     wait until they are done;
///     engage brakes if the flag was set.
/// </summary>
/// <param name="axes_names"></param>
/// <param name="positions"></param>
/// <param name="engageBrakes"></param>
/// <returns></returns>
TEKNIC_MOTOR_API_CODE TeknicMotorApi::move(const std::vector<std::string> axes_names, const std::vector<double> positions,
    const bool engageBrakes)
{
    // dev rec: if you want to move only some of the axes, make a subsection of axes and run move with them
    // do not make copies of this move function
    if (!wasInitializedCorrectly())
        return TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR;

    if (isEngaged())
        return TEKNIC_MOTOR_API_CODE::ENGAGED;

    engaged = true;

    disengageBrakes();

    string buf;
    TEKNIC_MOTOR_API_CODE answ = TEKNIC_MOTOR_API_CODE::OK;
    auto startTime = Times::getCurrentTime();

    // clear moves - stops all moves and clears the registry
    clearMoves();

    // go axis by axis
    // set triggers for advanced axes and execute moves on basic axes
    bool flagFoundCommand;
    double targetPositionCommand;
    for (auto& axis : axes)
    {
        // check timeout
        if (Times::isTimeout(startTime, action_timeout)) {
            stop();  // this will mark all movements as done
            buf = "Motors timed out waiting for moving to complete.";
            logError(buf.c_str());
            answ = TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
            break;
        }

        // find if there is a command to move
        flagFoundCommand = false;
        for (size_t i = 0; i < axes_names.size(); i++)
        {
            if (axis->name == axes_names[i]) {
                flagFoundCommand = true;
                targetPositionCommand = positions[i];
                break;
            }
        }
        if (!flagFoundCommand)  // no command for this axis
            continue;

        // if advanced, prepare advanced triggered position move - 
        // settings in the beginning enforce all advanced axes to be in a group
        if (axis->areAllAdvanced()) {
            answ = axis->prepareAdvancedMovement(targetPositionCommand);
        }
        else {  // if not advanced, move it using basic functions
            answ = axis->moveBasic(targetPositionCommand);
        }

        // check for errors
        if (isError(answ)) {
            stop();
            buf = "Motors encountered an error during move preparation. Aborting.";
            logError(buf.c_str());
        }
    }

    if (!isError(answ)) {
        // go group by group, trigger move and wait for it to complete
        for (size_t i_port = 0; i_port < portCount; i_port++)
        {
            for (int group_number : m_groups_in_ports[i_port]) {
                try {
                    m_manager->Ports(i_port).Adv.TriggerMovesInGroup(group_number);
                }
                catch (mnErr& theErr) {
                    stop();
                    buf = string("Adv.TriggerMovesInGroup error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
                    logError(buf.c_str());
                    answ = TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR;
                    break;
                }

                // while not done
                while (!isGroupMoveDone(group_number)) {
                    // check timeout
                    if (Times::isTimeout(startTime, action_timeout)) {
                        stop();  // this will mark all movements as done and clear move buffer
                        buf = "Motors timed out waiting for moving to complete.";
                        logError(buf.c_str());
                        answ = TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT;
                    }
                }
            }

            if (isError(answ))
                break;
        }
    }

    if (engageBrakes)
        this->engageBrakes();

    if (!isError(answ)) {
        logInfo("Move complete.");
    }

    engaged = false;

    return answ;
}

TEKNIC_MOTOR_API_CODE TeknicMotorApi::preshape(const std::vector<std::string> axes_names, const std::vector<double> positions)
{
    if (!wasInitializedCorrectly())
        return TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR;

    // make a list of axes and position to move
    vector<string> preshape_axes_names;
    vector<double> preshape_positions;

    // maintain order of the axes_names
    for (size_t i_axis = 0; i_axis < axes_names.size(); i_axis++)
    {
        for (auto& axis : axes) {
            if (axis->name == axes_names[i_axis]) {
                // found the axis with correct name
                if (!axis->retreat_axis) {
                    preshape_axes_names.push_back(axis->name);
                    preshape_positions.push_back(positions[i_axis]);
                }
                break;
            }
        }
    }

    return move(preshape_axes_names, preshape_positions, false);  // don't engage brakes after preshape
}

TEKNIC_MOTOR_API_CODE TeknicMotorApi::approach(const std::vector<std::string> axes_names, const std::vector<double> positions)
{
    if (!wasInitializedCorrectly())
        return TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR;

    // make a list of axes and position to move
    vector<string> approach_axes_names;
    vector<double> approach_positions;

    // maintain order of the axes_names
    for (size_t i_axis = 0; i_axis < axes_names.size(); i_axis++)
    {
        for (auto& axis : axes) {
            if (axis->name == axes_names[i_axis]) {
                // found the axis with correct name
                if (axis->retreat_axis) {
                    approach_axes_names.push_back(axis->name);
                    approach_positions.push_back(positions[i_axis]);
                }
                break;
            }
        }
    }

    return move(approach_axes_names, approach_positions, true);  // engage brakes after approach
}

void TeknicMotorApi::stop()
{
    for (auto& axis : axes)
        axis->stop();
}

std::vector<TEKNIC_MOTOR_API_CODE> TeknicMotorApi::checkPosition(const std::vector<std::string> axes_names, std::vector<double>& positions)
{
    std::vector<TEKNIC_MOTOR_API_CODE> answ;
    if (!wasInitializedCorrectly())
        return answ;

    double pos;
    for (size_t i_axis = 0; i_axis < axes_names.size(); i_axis++)
    {
        answ.push_back(TEKNIC_MOTOR_API_CODE::RANGE_INCONSISTENT);
        pos = positions[i_axis];
        for (auto& axis : axes)
        {
            if (axis->name == axes_names[i_axis]) {
                answ[i_axis] = axis->putInRange(pos);
                positions[i_axis] = pos;
                break;
            }
        }
    }
    return answ;
}

bool TeknicMotorApi::wasInitializedCorrectly()
{
    return initializedCorrectly;
}

bool TeknicMotorApi::wereHomed()
{
    bool answ = true;
    for (auto& axis : axes)
        answ = answ && axis->wereAllHomed();  // all
    return answ;
}

bool TeknicMotorApi::wereHomed(const std::vector<std::string> axes_names)
{
    bool answ = true;
    bool found;
    for (auto& axis : axes) {
        found = false;
        for (auto& axis_name : axes_names)
            if (!axis_name.compare(axis->name)) {
                found = true;
                break;
            }
        if (found)
            answ = answ && axis->wereAllHomed();  // all
    }
    return answ;
}

bool TeknicMotorApi::isEngaged()
{
    return engaged;
}

/// <summary>
/// Checks if the API code was an error (ret true) or OK or warning (ret false).
/// 
/// Errors usually mean that the function did not complete its primary purpose and any later
/// code that depends on it should abort.
/// </summary>
/// <param name="code"></param>
/// <returns></returns>
const bool TeknicMotorApi::isError(const TEKNIC_MOTOR_API_CODE& code)
{
    bool answ = true;
    switch (code)
    {
    case TEKNIC_MOTOR_API_CODE::OK:
    case TEKNIC_MOTOR_API_CODE::MOTOR_DISABLED_BY_CONFIG:
    case TEKNIC_MOTOR_API_CODE::MOTOR_ALREADY_ENABLED:
    case TEKNIC_MOTOR_API_CODE::NO_MOTORS_ON_AXIS:
        answ = false;
        break;
    case TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT:
    case TEKNIC_MOTOR_API_CODE::MOTOR_HOMING_INVALID:
    case TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR:
    case TEKNIC_MOTOR_API_CODE::COULD_NOT_ENABLE_MOTOR:
    case TEKNIC_MOTOR_API_CODE::ACTION_COMPLETED_INCORRECTLY:
    case TEKNIC_MOTOR_API_CODE::DOES_NOT_SUPPORT_FUNCTION:
    case TEKNIC_MOTOR_API_CODE::USER_INTERRUPT:
    case TEKNIC_MOTOR_API_CODE::RANGE_INCONSISTENT:
    case TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR:
    case TEKNIC_MOTOR_API_CODE::ENGAGED:
    case TEKNIC_MOTOR_API_CODE::JSON_PARSING_ERROR:
        answ = true;
        break;
    default:
        std::string buf = "Unknown error code ID " + to_string(static_cast<int>(code)) + ".";
        logError(buf.c_str());
        break;
    }
    return answ;
}

const std::string TeknicMotorApi::codeMessage(const TEKNIC_MOTOR_API_CODE& code)
{
    std::string answ;
    switch (code)
    {
    case TEKNIC_MOTOR_API_CODE::OK:
        answ = "OK.";
        break;
    case TEKNIC_MOTOR_API_CODE::MOTOR_DISABLED_BY_CONFIG:
        answ = "Motor disabled by config";
        break;
    case TEKNIC_MOTOR_API_CODE::MOTOR_ALREADY_ENABLED:
        answ = "Motor already enabled.";
        break;
    case TEKNIC_MOTOR_API_CODE::NO_MOTORS_ON_AXIS:
        answ = "No motors on axis.";
        break;
    case TEKNIC_MOTOR_API_CODE::ACTION_TIMEOUT:
        answ = "Action timeout.";
        break;
    case TEKNIC_MOTOR_API_CODE::MOTOR_HOMING_INVALID:
        answ = "Motor homing invalid.";
        break;
    case TEKNIC_MOTOR_API_CODE::TEKNIC_ERROR:
        answ = "Teknic error.";
        break;
    case TEKNIC_MOTOR_API_CODE::COULD_NOT_ENABLE_MOTOR:
        answ = "Could not enable motor.";
        break;
    case TEKNIC_MOTOR_API_CODE::ACTION_COMPLETED_INCORRECTLY:
        answ = "Action completed incorrectly.";
        break;
    case TEKNIC_MOTOR_API_CODE::DOES_NOT_SUPPORT_FUNCTION:
        answ = "Does not support function.";
        break;
    case TEKNIC_MOTOR_API_CODE::USER_INTERRUPT:
        answ = "User interrupt.";
        break;
    case TEKNIC_MOTOR_API_CODE::RANGE_INCONSISTENT:
        answ = "Range inconsistent.";
        break;
    case TEKNIC_MOTOR_API_CODE::INITIALIZATION_ERROR:
        answ = "Initialization error.";
        break;
    case TEKNIC_MOTOR_API_CODE::ENGAGED:
        answ = "Engaged.";
        break;
    case TEKNIC_MOTOR_API_CODE::JSON_PARSING_ERROR:
        answ = "JSON parsing error.";
        break;
    default:
        answ = "Unknown error code ID " + to_string(static_cast<int>(code)) + ".";
        logError(answ.c_str());
        break;
    }

    return answ;
}

std::vector<std::string> TeknicMotorApi::getAxesNames()
{
    std::vector<std::string> answ;
    if (!wasInitializedCorrectly())
        return answ;

    for (auto& axis : axes)
        answ.push_back(axis->name);
    return answ;
}

std::vector<std::string> TeknicMotorApi::getAxesUnits()
{
    std::vector<std::string> answ;
    if (!wasInitializedCorrectly())
        return answ;

    for (auto& axis : axes)
        answ.push_back(axis->units);
    return answ;
}

std::string TeknicMotorApi::getReport()
{
    std::string answ;
    for (auto& axis : axes) {
        answ += axis->getReport();
    }
    return answ;
}

void TeknicMotorApi::engageBrakes()
{
    for (int i_port = 0; i_port < portCount;i_port++) {
        m_manager->Ports(i_port).BrakeControl.BrakeSetting(0, BRAKE_PREVENT_MOTION);
        m_manager->Ports(i_port).BrakeControl.BrakeSetting(1, BRAKE_PREVENT_MOTION);
    }
}

void TeknicMotorApi::disengageBrakes()
{
    for (int i_port = 0; i_port < portCount; i_port++) {
        m_manager->Ports(i_port).BrakeControl.BrakeSetting(0, BRAKE_ALLOW_MOTION);
        m_manager->Ports(i_port).BrakeControl.BrakeSetting(1, BRAKE_ALLOW_MOTION);
    }
}

void TeknicMotorApi::clearMoves()
{
    for (auto& axis : axes)
        axis->clearMoves();
}

bool TeknicMotorApi::areAllMovesDone()
{
    bool allMovesDone = true;
    for (auto& axis : axes)
        allMovesDone = allMovesDone && axis->isAllMoveDone();
    return allMovesDone;
}

bool TeknicMotorApi::isGroupMoveDone(const int group_number)
{
    bool allgroupMovesDone = true;
    for (auto& axis : axes)
        if (axis->getParallelGroup() == group_number)
            allgroupMovesDone = allgroupMovesDone && axis->isAllMoveDone();
    return allgroupMovesDone;
}

int TeknicMotorApi::loadAxesConfiguration(const std::string axes_config_filename)
{
    string buf;

    // load settings file
    ifstream ifs(axes_config_filename);
    if (ifs.fail()) {
        buf = "Could not open motor axes settings JSON file: " + axes_config_filename + ". Check if it exists.";
        logError(buf.c_str());
        return -1;
    }

    // parse it
    json axes_json;
    try
    {
        axes_json = json::parse(ifs);
    }
    catch (const json::parse_error& e)
    {
        buf = "Parse error during motor axes settings JSON file read: " + string(e.what());
        logError(buf.c_str());
        return -1;
    }

    this->axes.clear();
    for (size_t i = 0; i < axes_json.size(); i++)
    {
        try
        {
            json axis = axes_json.at(to_string(i).c_str());
            this->axes.push_back(make_unique<Axis>(axis));
        }
        catch (const json::exception& e) 
        {
            buf = "Problems reading JSON axes file at entry " + to_string(i) + ". Error:" + string(e.what());
            logError(buf.c_str());
            this->axes.clear();
            return -1;
        }
    }

    return 0;
}
