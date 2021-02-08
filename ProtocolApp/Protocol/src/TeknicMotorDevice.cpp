#include "TeknicMotorDevice.h"

using namespace sFnd;
using namespace std;


//------------------------------------------------------------------------------------
//--------------------------------------- CONVERTOR ----------------------------------
//------------------------------------------------------------------------------------

Convertor::Convertor(const int _min_out, const int _max_out, const double _in_offset, const double _in_to_out_factor, const double _sign)
{
    min_out = _min_out;
    max_out = _max_out;
    in_offset = _in_offset;
    in_to_out_factor = _in_to_out_factor;
    sign = _sign;
}

Convertor::~Convertor()
{
}

int Convertor::convert(const int val)
{
    return convert((double) val);
}

int Convertor::convert(const double val)
{
    int answ = (int)round((sign * val + in_offset) * in_to_out_factor);

    if (answ < min_out)
        answ = min_out;

    if (answ > max_out)
        answ = max_out;

    return answ;
}

//------------------------------------------------------------------------------------
//--------------------------------------- NODE ---------------------------------------
//------------------------------------------------------------------------------------

/* 
* Wrapper for interface
*/
Node::Node(sFnd::INode* node, const int index) :
    m_node(node) {

    disable();
    // set units for velocity and acceleration
    m_node->VelUnit(INode::RPM);
    m_node->AccUnit(INode::RPM_PER_SEC);

    retreat_position = 0;
    default_vel = 1;
    default_acc = 1;
    // define the node conversion modules and default values  
    switch (index)
    {
    case 0:
        m_node->Setup.ConfigLoad("./configuration/motor_translation_z.mtr");
        // MOTOR CPM-SCHP-3441S-ELSB-1-7-D
        // https://www.teknic.com/model-info/CPM-SCHP-3441S-ELSB/

        pos = new Convertor(-90000, 10000, 0, -75000. / 150); // input in mm
        // max is 840 RPM @ 75 VDC
        vel = new Convertor(1, 840, 0, 840. / 10);  // arbitrary scaling factor to RPM
        // st/s^2=10'546'000 ? <- possibly wrong calculation from Teknic
        acc = new Convertor(1, 8400, 0, 8400. / 10);  // arbitrary scaling factor to RPM/s

        default_vel = 2;

        break;
    case 1:
        m_node->Setup.ConfigLoad("./configuration/motor_tilt.mtr");
        // MOTOR CPM-SCHP-3421S-ELSA-1-7-D
        // https://www.teknic.com/model-info/CPM-SCHP-3421S-ELSA/

        pos = new Convertor(-650, 650, 0, 500. / 45); // input in degrees
        // max is 1410 RPM @ 75 VDC
        vel = new Convertor(1, 1410, 0, 1410. / 10);  // arbitrary scaling factor to RPM
        acc = new Convertor(1, 14100, 0, 14100. / 10);  // arbitrary scaling factor to RPM/s

        default_vel = 0.03;

        break;
    case 2:
        m_node->Setup.ConfigLoad("./configuration/motor_aperture.mtr");
        // MOTOR CPM-SCHP-2311S-ELSA-1-7-D
        // https://www.teknic.com/model-info/CPM-SCHP-2311S-ELSA/

        // Homed against the widest position - 35 mm width
        pos = new Convertor(-500, 36000, 35, 35000. / 35, -1); // input in mm
        // max is 4000 RPM @ 75 VDC
        vel = new Convertor(1, 4000, 0, 4000. / 10);  // arbitrary scaling factor to RPM
        acc = new Convertor(1, 40000, 0, 40000. / 10);  // arbitrary scaling factor to RPM/s

        default_vel = 0.1;

        break;
    default:
        string buf = "TeknicMotorDevice: Error during Node initialization: Unknown index: ";
        buf += to_string(index) + ".";
        logError(buf.c_str());
        throw invalid_argument("Error during Node initialization: Unknown name.");
        break;
    }

    // name loaded with settings
    m_name = m_node->Info.UserID.Value();

    printDetails();
    enable();  // waits until the node is enabled
}

Node::~Node(void) {
    // disabled by the API
    // disable();
}



/* The following statements will attempt to enable the node. First, any
shutdowns or NodeStops are cleared, finally the node is enabled */

int Node::enable() {
    handleAlerts();
    // allows movement if a stop was used before
    clearAlertsNodeStops();
    m_node->EnableReq(true);

    string buf;

    //define a timeout in case the node is unable to enable
    auto startTimeoutTime = Times::getCurrentTime();
    //This will loop checking on the Real time values of the node's Ready status
    while (!m_node->Motion.IsReady()) {
        if (Times::isTimeout(startTimeoutTime, action_timeout)) {
            buf = "TeknicMotorDevice Error: Timed out waiting for Node " + m_name + " to enable.";
            logError(buf.c_str());
            return -1;
        }
    }
    buf = "Node enabled " + m_name + ".";
    logInfo(buf.c_str());
    return 0;
}


void Node::disable() {
    try
    {
        m_node->EnableReq(false);
    }
    catch (mnErr& theErr) {
        string buf = string("home disable error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
    }
}


/* Find home position of the node. */
int Node::home() {
    if (enable() < 0)
        return -4;

    string buf;
    if (m_node->Motion.Homing.HomingValid()) {
        if (m_node->Motion.Homing.WasHomed()) {
            buf = "Node has already been homed, current position is:" + to_string(m_node->Motion.PosnMeasured.Value());
            logInfo(buf.c_str());
        }
        else {
            logInfo("Node has not been homed.");
        }
        logInfo("Homing Node now...");
        try
        {
            m_node->Motion.Homing.Initiate();
        }
        catch (mnErr& theErr) {
            buf = string("home Node error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
            logError(buf.c_str());
            return -3;
        }

        // define a timeout in case the node is unable to home
        auto startTimeoutTime = Times::getCurrentTime();
        while (!m_node->Motion.Homing.WasHomed()) {
            if (Times::isTimeout(startTimeoutTime, action_timeout)) {
                logError("Node did not complete homing:  ");
                logError("\t -Ensure Homing settings have been defined through ClearView.");
                logError("\t -Check for alerts/Shutdowns");
                logError("\t - Ensure timeout is longer than the longest possible homing move.");
                return -2;
            }
        }
        logInfo("Node completed homing.");
    }
    else {
        buf = "TeknicMotorDevice Error: Homing never setup through ClearView. Node " + m_name + " cannot be homed.";
        logError(buf.c_str());
        return -1;
    }
    return 0;
}


/* Diagnostics print. */
void Node::printDetails() {
    std::string nType;
    switch (m_node->Info.NodeType()) {
    case IInfo::MERIDIAN_ISC:
        nType = "MERIDIAN_ISC";
        break;
    case IInfo::CLEARPATH_SC:
        nType = "CLEARPATH_SC";
        break;
    case IInfo::CLEARPATH_SC_ADV:
        nType = "CLEARPATH_SC_ADV";
        break;
    case IInfo::UNKNOWN:
    default:
        nType = "UNKNOWN";
        break;
    }

    string buf;

    buf = "  NodeType: " + nType;
    logInfo(buf.c_str());
    buf = string("     Model: ") + m_node->Info.Model.Value();
    logInfo(buf.c_str());
    buf = "  Serial #: " + to_string(m_node->Info.SerialNumber.Value());
    logInfo(buf.c_str());
    buf = string("FW version: ") + m_node->Info.FirmwareVersion.Value();
    logInfo(buf.c_str());
    buf = "    userID: " + m_name;
    logInfo(buf.c_str());
}

void Node::clearAlertsNodeStops()
{
    m_node->Status.AlertsClear();
    m_node->Motion.NodeStopClear();
}

void Node::handleAlerts() {
    // Buffer for possible messages.
    char alertList[256];
    string buf;

    m_node->Status.RT.Refresh();
    m_node->Status.Alerts.Refresh();

    // if an alert is present:
    if (!m_node->Status.RT.Value().cpm.AlertPresent) {

        if (m_node->Status.Alerts.Value().isInAlert()) {
            // get a copy of the alert register bits and a text description of all bits set
            m_node->Status.Alerts.Value().StateStr(alertList, 256);
            buf = string("Alerts found on ") + m_name + " node: " + alertList;
            logWarning(buf.c_str());
        }
    }

    //Check to see if the node experienced torque saturation
    if (m_node->Status.HadTorqueSaturation()) {
        buf = "Node ";
        buf += m_name + " has experienced torque saturation since last checking";
        logWarning(buf.c_str());
    }
}

bool Node::isMoveDone()
{
    return m_node->Motion.MoveIsDone();
}

bool Node::wasHomed()
{
    if (m_node->Motion.Homing.HomingValid()) {
        if (m_node->Motion.Homing.WasHomed())
            return true;
    }
    else {
        string buf = "TeknicMotorDevice Error: Homing never setup through ClearView. Should not be asking if Node " + m_name + " was homed.";
        logWarning(buf.c_str());
    }
    return false;
}

/// <summary>
/// Generic move function built off examples. NOT USED. Use parallel one from API
/// </summary>
/// <param name="moveCounts"></param>
/// <param name="speed"></param>
/// <param name="accel"></param>
void Node::m_move(const int& moveCounts, const double& speed, const double& accel,
    atomic<bool>* stopTrial, atomic<bool>* stopProtocol) {
    // enables and handles alerts
    if (enable() < 0)
        return;

    string buf;
    int relativeMoveCounts = (int)round(moveCounts - m_node->Motion.PosnMeasured.Value());

    // Then set the velocity/accel:
    m_node->Motion.VelLimit = speed;
    m_node->Motion.AccLimit = accel;

    // Now move.
    buf = "Moving Node " + m_name + " moveCounts " + to_string(moveCounts) + " relative " + to_string(relativeMoveCounts);
    logInfo(buf.c_str());

    // start the movement, runs asynchronously
    try {
        m_node->Motion.MovePosnStart(moveCounts, true);
    }
    catch (mnErr& theErr) {
        buf = string("move Node error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
        return;
    }

    //// some log
    //auto moveTime = m_node->Motion.MovePosnDurationMsec(moveCounts, true);
    //buf = "Estimated move duration (abs): " + to_string(moveTime) + "ms";
    //logInfo(buf.c_str());

    // lock the execution until the move is done or a timeout occured
    auto startTimeoutTime = Times::getCurrentTime();
    while (!isMoveDone()) {
        if (Times::isTimeout(startTimeoutTime, action_timeout)) {
            logError("Timed out waiting for move to complete");
            stop();  // interrupts the movement and makes moveisdone true
        }

        if (stopProtocol->load() || stopTrial->load()) {
            stop();
        }
    }
    buf = "Move complete on " + m_name;
    logInfo(buf.c_str());
    //! Clear the register only if it's successful?
    // m_node->Motion.MoveWentDone();        // Clear "move done" register
}

int Node::m_initiateMove(const int& moveCounts, const double& speed, const double& accel)
{
    // enables and handles alerts
    if (enable() < 0)
        return -4;

    string buf;
    // calculate how much you need to move
    int relativeMoveCounts = (int)round(moveCounts - m_node->Motion.PosnMeasured.Value());

    // Then set the velocity/accel:
    m_node->Motion.VelLimit = speed;
    m_node->Motion.AccLimit = accel;

    // Now move.
    buf = "Moving Node " + m_name + " moveCounts " + to_string(moveCounts) + " relative " + to_string(relativeMoveCounts);
    logInfo(buf.c_str());

    // start the movement, runs asynchronously
    int answ = 0;
    try {
        m_node->Motion.MovePosnStart(relativeMoveCounts);
    }
    catch (mnErr& theErr) {
        buf = string("move Node error [") + to_string(theErr.TheAddr) + "] " + theErr.ErrorMsg;
        logError(buf.c_str());
        answ = -1;
    }
    return answ;
}


/// <summary>
/// High level move function built to convert from human units to machine.
/// </summary>
/// <param name="position">unit depends on Convert member variables</param>
/// <param name="velLevel"></param>
/// <param name="accLevel"></param>
void Node::move(const double& position, const double& velLevel, const double& accLevel,
    std::atomic<bool>* stopTrial, std::atomic<bool>* stopProtocol) {
    m_move(pos->convert(position), vel->convert(velLevel), acc->convert(accLevel),
        stopTrial, stopProtocol);
}

void Node::move(const double& position,
    std::atomic<bool>* stopTrial, std::atomic<bool>* stopProtocol)
{
    move(position, default_vel, default_acc,
        stopTrial, stopProtocol);
}

void Node::retreat()
{
    atomic<bool> stopTrial = false;
    atomic<bool> stopProtocol = false;
    move(retreat_position, &stopTrial, &stopProtocol);
}

void Node::stop()
{
    m_node->Motion.NodeStop(STOP_TYPE_ESTOP_ABRUPT);
}

int Node::initiateMove(const double& position, const int& velLevel, const int& accLevel)
{
    return m_initiateMove(pos->convert(position), vel->convert(velLevel), acc->convert(accLevel));
}

int Node::initiateMove(const double& position)
{
    return initiateMove(position, default_vel, default_acc);
}

int Node::initiateRetreat()
{
    return initiateMove(retreat_position, default_vel, default_acc);
}



//------------------------------------------------------------------------------------
//--------------------------------------- MotorAPI -----------------------------------
//------------------------------------------------------------------------------------

/**
 * Figure out how many SC4-HUBs are daisy chained ([0-2] per port), and how many
 * servo motors (nodes) are plugged into them. Register them to the SysManager.
 *
 * Ports are literal COM serial ports, COM6 by default on ours.
 *
 * Will enable and home all nodes.
 *
 * (can try to load config file for each motor.)
 */
MotorAPI::MotorAPI(void) {
    initializedCorrectly = false;

    std::vector<std::string> comHubPorts;
    string buf;

    // get instance and find ports
    try {
        m_manager = SysManager::Instance();
        m_manager->FindComHubPorts(comHubPorts);
    }
    catch (mnErr& theErr) {
        // (defined by the mnErr class)
        buf = (string("MotorAPI() constructor Sys manager and FindComPorts | addr: ") + to_string(theErr.TheAddr) +
            " | err: " + to_string(theErr.ErrorCode) + " | msg: " + theErr.ErrorMsg);
        logError(buf.c_str());
    }

    // check if com ports are present
    if (comHubPorts.empty()) {
        logError("TeknicMotorDevice Error: No SC Hubs found! Motors not enabled.");
        //exit(1); // AS: exit is a very bad practice
        return;
    }

    // Log the number of ports found
    size_t portCount = comHubPorts.size();
    buf = "Found " + to_string(portCount) + " SC Hubs";
    logInfo(buf.c_str());

    // set a local serial number id to the named com port
    for (size_t i = 0; i < portCount && i < NET_CONTROLLER_MAX; i++)
        m_manager->ComHubPort(i, comHubPorts[i].c_str());

    try {
        m_manager->PortsOpen(portCount);
    }
    catch (mnErr& theErr) {
        // (defined by the mnErr class)
        buf = ("MotorAPI() constructor PortsOpen | addr: " + to_string(theErr.TheAddr) +
            " | err: " + to_string(theErr.ErrorCode) + " | msg: " + theErr.ErrorMsg);
        logError(buf.c_str());
    }

    // Drop all of our port references into private array
    for (size_t i = 0; i < portCount; i++)
        m_ports.push_back(&m_manager->Ports(i));

    // Initialize nodes. Have to iterate ports, then nodes per port
    // I only expect one port currently, but this is for safety.

    buf = "Iterating through " + to_string(portCount) + " nodes";
    logInfo(buf.c_str());

    for (size_t i = 0; i < portCount; i++) { // For each port:
        buf = "Iterating nodes on port " + to_string(i) + ".";
        logInfo(buf.c_str());
        IPort* thisPort = m_ports[i];

        // Turn off all motors when we initialize the interfaces.
        // AS: not sure what ^ means in context

        // Makes both breaks on the port allow motion
        thisPort->BrakeControl.BrakeSetting(0, BRAKE_ALLOW_MOTION);
        thisPort->BrakeControl.BrakeSetting(1, BRAKE_ALLOW_MOTION);

        // log
        buf = ("Port: " + to_string(thisPort->NetNumber()) + 
            ", State: " + to_string(thisPort->OpenState()) +
            ", Node count: " + to_string(thisPort->NodeCount()) + ".");
        logInfo(buf.c_str());

        // Iterate nodes on this port
        for (int nodeIndex = 0; nodeIndex < thisPort->NodeCount(); nodeIndex++) {
            // shield error throw
            try
            {
                m_nodes.push_back(Node(&(thisPort->Nodes(nodeIndex)), nodeIndex));
            }
            catch (const mnErr& theErr)
            {
                buf = ("Error during Node " + to_string(nodeIndex) +
                    " initialization. Error message: " + theErr.ErrorMsg);
                logError(buf.c_str());
                return;
            }
            catch (const invalid_argument& ia) {
                return;
            }
        }
    }

    engageBrakes();

    initializedCorrectly = true;
}

MotorAPI::~MotorAPI(void) {
    logInfo("Teknic Shutting down. Disabling nodes, and closing port");
    for (auto e : m_nodes) {
        e.disable();
    }
    if (wasInitializedCorrectly())
        m_manager->PortsClose();
    logInfo("Teknic Shutdown!");
}

int MotorAPI::home()
{
    // Done in sequence, why not
    if (!wasInitializedCorrectly())
        return -1;

    disengageBrakes();

    int answ = 0;
    for (auto node : m_nodes) {
        answ += node.home();
    }
    //engageBrakes();

    return answ;
}

int MotorAPI::retreat()
{
    // all movements are initialized in parallel, and finish watched together here.
    if (!wasInitializedCorrectly())
        return -1;

    disengageBrakes();
    logInfo("\tStarting retreat.");

    auto startTime = Times::getCurrentTime();
    int answ = 0;
    if (retreat_node_i < 0 || retreat_node_i >= m_nodes.size()) {
        for (size_t i = 0; i < m_nodes.size(); i++) {
            // check timeout
            if (Times::isTimeout(startTime, action_timeout)) {
                answ = -4;
                break;
            }

            m_nodes[i].retreat();
        }
    }
    else {
        m_nodes[retreat_node_i].retreat();
    }

    //engageBrakes();
    if (!answ) {
        logInfo("Retreat complete.");
    }

    return answ;
}

int MotorAPI::move(std::vector<double> positions, std::atomic<bool>* stopTrial, std::atomic<bool>* stopProtocol)
{
    // all movements are initialized in parallel, and finish watched together here.
    if (!wasInitializedCorrectly())
        return -1;

    if (positions.size() > m_nodes.size())
        return -2;

    disengageBrakes();
    logInfo("\tStarting move.");

    auto startTime = Times::getCurrentTime();
    int answ = 0;
    for (int i = positions.size() - 1; i >= 0; i--) {
        // check if trial or the whole protocol has been signalled to stop
        if (stopTrial->load() || stopProtocol->load()) {
            stop();  // this will mark all movements as done
        }

        // check timeout
        if (Times::isTimeout(startTime, action_timeout)) {
            answ = -4;
            break;
        }

        m_nodes[i].move(positions[i], stopTrial, stopProtocol);
    }

    engageBrakes();

    if (!answ) {
        logInfo("Move complete.");
    }

    return answ;
}

void MotorAPI::stop()
{
    mtx.lock();
    for (auto e : m_nodes)
        e.stop();
    mtx.unlock();
}

bool MotorAPI::wasInitializedCorrectly()
{
    return initializedCorrectly;
}

bool MotorAPI::wereHomed()
{
    bool answ = true;
    for (auto e : m_nodes)
        answ = answ && e.wasHomed();  // any
    return answ;
}

void MotorAPI::setActionTimeout(double timeSecs)
{
    action_timeout = timeSecs;
    for (auto e : m_nodes)
        e.action_timeout = timeSecs;
}

void MotorAPI::engageBrakes()
{
    for (auto e : m_ports) {
        e->BrakeControl.BrakeSetting(0, BRAKE_PREVENT_MOTION);
        e->BrakeControl.BrakeSetting(1, BRAKE_PREVENT_MOTION);
    }
}

void MotorAPI::disengageBrakes()
{
    for (auto e : m_ports) {
        e->BrakeControl.BrakeSetting(0, BRAKE_ALLOW_MOTION);
        e->BrakeControl.BrakeSetting(1, BRAKE_ALLOW_MOTION);
    }
}
