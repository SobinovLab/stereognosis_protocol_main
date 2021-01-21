/**
 * @file motorAPI.cpp
 *
 * @brief gRPC API implemented for Teknic Clearpath motors.
 *
 * @author Danielle MacDonald
 * Contact: dmacd@uchicago.edu
 * 
 * Modified: Anton Sobinov
 */


#include "TeknicMotorDevice.h"

//The timeout used for homing and moving (ms)
#define HOMING_TIMEOUT		    10000

using namespace sFnd;
using namespace std;

//------------------------------------------------------------------------------------
//--------------------------------------- NODE ---------------------------------------
//------------------------------------------------------------------------------------

/* 
* Wrapper for interface
*/
Node::Node(sFnd::INode& node, string type) :
    m_node(node) {

    std::string m_name = m_node.Info.UserID.Value();

    if (type == "forward_backward" || type == "default") {  // serial node 0
        pos = new Convertor(0.1, 240, -105000); // input in mm
        vel = new Convertor(1, 10, 700);  // arbitrary scaling factor to RPM
        acc = new Convertor(1, 10, 4000);  // arbitrary scaling factor to RPM/s

        retreat_position = 1;  // TODO check attilio's default value
        default_vel = 2;
        default_acc = 2;
    }
    else if (type == "tilt") {  // serial node 1
        // TODO not implemented
        pos = new Convertor(0.1, 240, -105000); // input in degrees
        vel = new Convertor(1, 10, 700);  // arbitrary scaling factor to RPM
        acc = new Convertor(1, 10, 4000);  // arbitrary scaling factor to RPM/s

        retreat_position = 1;
        default_vel = 2;
        default_acc = 2;
    }
    else if (type == "aperture") {  // serial node 2
        // TODO not implemented
        pos = new Convertor(0.1, 240, -105000); // input in degrees
        vel = new Convertor(1, 10, 700);  // arbitrary scaling factor to RPM
        acc = new Convertor(1, 10, 4000);  // arbitrary scaling factor to RPM/s

        retreat_position = 1;
        default_vel = 2;
        default_acc = 2;
    }
    else {
        logError("Error during Node initialization: Unknown type.");
        throw invalid_argument("Error during Node initialization: Unknown type.");
    }

    // Following 3 are optional if I wish to load a config file:
    // thisNode.EnableReq(false); // Should disable Node before loading config
    // m_manager->Delay(200);     //? sleep (ms?) to make sure disable is registered?
    // theNode.Setup.ConfigLoad("Config File path");
    printDetails();
    enable();
    //! home();
}

Node::~Node(void) {
    m_node.EnableReq(false);
}



/* The following statements will attempt to enable the node. First, any
shutdowns or NodeStops are cleared, finally the node is enabled */

void Node::enable() {
    m_node.Status.AlertsClear();
    m_node.Motion.NodeStopClear();
    m_node.EnableReq(true);

    string buf;

    //define a timeout in case the node is unable to enable
    auto startTimeoutTime = Times::getCurrentTime();
    //This will loop checking on the Real time values of the node's Ready status
    while (!m_node.Motion.IsReady()) {
        if (Times::isTimeout(startTimeoutTime, (double)HOMING_TIMEOUT / 1000)) {
            buf = "Error: Timed out waiting for Node " + m_name + " to enable.";
            logError(buf.c_str());
            return;
        }
    }
    buf = "Node enabled " + m_name + ".";
    logInfo(buf.c_str());
}


void Node::disable() {
    m_node.EnableReq(false);
}


/* Find home position of the node. */
void Node::home() {
    string buf;
    if (m_node.Motion.Homing.HomingValid()) {
        if (m_node.Motion.Homing.WasHomed()) {
            buf = "Node has already been homed, current position is:" + to_string(m_node.Motion.PosnMeasured.Value());
            logInfo(buf.c_str());
        }
        else {
            logInfo("Node has not been homed.");
        }
        logInfo("Homing Node now...");
        m_node.Motion.Homing.Initiate();

        //define a timeout in case the node is unable to enable
        auto startTimeoutTime = Times::getCurrentTime();
        while (!m_node.Motion.Homing.WasHomed()) {
            if (Times::isTimeout(startTimeoutTime, (double)HOMING_TIMEOUT / 1000)) {
                logError("Node did not complete homing:  ");
                logError("\t -Ensure Homing settings have been defined through ClearView.");
                logError("\t -Check for alerts/Shutdowns");
                logError("\t - Ensure timeout is longer than the longest possible homing move.");
            }
        }
        logInfo("Node completed homing.");
    }
    else {
        buf = "Homing never setup through ClearView. Node " + m_name + " cannot be homed.";
        logInfo(buf.c_str());
    }
}


/* Diagnostics print. */
void Node::printDetails() {
    std::string nType = "CLEARPATH_SC";
    if (m_node.Info.NodeType() == 3) nType = "CLEARPATH_SC_ADV";

    string buf;

    buf = "  NodeType: " + nType;
    logInfo(buf.c_str());
    buf = string("     Model: ") + m_node.Info.Model.Value();
    logInfo(buf.c_str());
    buf = "  Serial #: " + to_string(m_node.Info.SerialNumber.Value());
    logInfo(buf.c_str());
    buf = string("FW version: ") + m_node.Info.FirmwareVersion.Value();
    logInfo(buf.c_str());
    buf = "    userID: " + m_name;
    logInfo(buf.c_str());
}

void Node::handleAlerts() {
    // Buffer for possible messages.
    char alertList[256];
    string buf;

    m_node.Status.RT.Refresh();
    m_node.Status.Alerts.Refresh();

    // if an alert is present:
    if (!m_node.Status.RT.Value().cpm.AlertPresent) {

        if (m_node.Status.Alerts.Value().isInAlert()) {
            // get a copy of the alert register bits and a text description of all bits set
            m_node.Status.Alerts.Value().StateStr(alertList, 256);
            buf = string("Alerts found on this node: ") + alertList;
            logWarning(buf.c_str());
        }
    }

    //Check to see if the node experienced torque saturation
    if (m_node.Status.HadTorqueSaturation()) {
        logWarning("Node has experienced torque saturation since last checking");
    }
}

/* Generic move function built off examples. */
void Node::move(
    const int& moveCounts,
    const int& speed,
    const int& accel
) {
    // Need to do some pre-checks to make sure node is ready:
    handleAlerts();

    string buf;

    // Then set the velocity/accel:
    m_node.VelUnit(sFnd::INode::RPM);
    m_node.Motion.VelLimit = speed;

    m_node.AccUnit(sFnd::INode::RPM_PER_SEC);
    m_node.Motion.AccLimit = accel;

    // Now move.
    buf = "Moving Node " + m_name + " moveCounts " + to_string(moveCounts);
    logInfo(buf.c_str());
    try {
        m_node.Motion.MovePosnStart(moveCounts);

        auto moveTime = m_node.Motion.MovePosnDurationMsec(moveCounts, true);
        buf = "Estimated move duration (abs): " + to_string(moveTime) + "ms";
        logInfo(buf.c_str());

        auto startTimeoutTime = Times::getCurrentTime();
        while (!m_node.Motion.MoveIsDone()) {
            // wait here for move
            if (Times::isTimeout(startTimeoutTime, (double)HOMING_TIMEOUT / 1000)) {
                logError("Timed out waiting for move to complete");
            }
        }
        buf = "Move complete on " + m_name;
        logInfo(buf.c_str());
        //! Clear the register only if it's successful?
        // m_node.Motion.MoveWentDone();        // Clear "move done" register
    }
    catch (sFnd::mnErr& theErr) {
        // (defined by the mnErr class)
        // sFnd::_mnErr::ErrorMsg
        buf = string("moveNode() [") + to_string(theErr.TheAddr) + "] " + to_string(theErr.ErrorCode) + "| " + theErr.ErrorMsg;
        logError(buf.c_str());
        // Some test cases to see if I can do inline remediation based on code:
        if (theErr.ErrorCode == MN_ERR_TIMEOUT) {
            logError("\tGot timeout.");
        }
        if (theErr.ErrorCode == MN_ERR_CMD_MV_BLOCKED) {
            logError("\tMove blocked.");
        }
    }
}


/* High level move function built to convert from human units to machine. */
void Node::moveHigh(
    const int& position,  // unit depends on Convert members
    const int& velLevel,
    const int& accLevel
) {
    move(pos->convert(position), vel->convert(velLevel), acc->convert(accLevel));
}

void Node::moveHigh(const int& position)
{
    moveHigh(position, default_vel, default_acc);
}

void Node::retreat()
{
    moveHigh(retreat_position);
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
    // The example just declares it default, says it's singleton
    // AS: for a singleton, creator and destructor should be private, all access to the object is done through a static function, 
    //     for example, see SysManager
    std::vector<std::string> comHubPorts;
    string buf;

    //TODO very large try-catch, needs to be smaller to be useful
    try {
        m_manager = SysManager::Instance();
        m_manager->FindComHubPorts(comHubPorts);
        if (comHubPorts.empty()) {
            logError("No SC Hubs found! Exiting.");  // TODO return instead of exit?
            //exit(1); // AS: exit is a very bad practice
            return;
        }
        m_portCount = comHubPorts.size();
        buf = "Found " + to_string(m_portCount) + " SC Hubs";
        logInfo(buf.c_str());

        for (size_t i = 0; i < m_portCount && i < NET_CONTROLLER_MAX; i++)
            m_manager->ComHubPort(i, comHubPorts[i].c_str());

        m_manager->PortsOpen(m_portCount);

        // Drop all of our port references into my own array
        for (size_t i = 0; i < m_portCount; i++)
            m_ports.push_back(std::reference_wrapper<IPort>(m_manager->Ports(i)));

        // Initialize nodes. Have to iterate ports, then nodes per port
        //? I only expect one port currently, but this is for safety.

        buf = "Iterating through " + to_string(m_portCount) + " nodes";
        logInfo(buf.c_str());

        for (size_t i = 0; i < m_portCount; i++) { // For each port:
            buf = "Iterating nodes on port " + to_string(i) + ".";
            logInfo(buf.c_str());
            IPort& thisPort = m_ports[i].get();

            // Turn off all motors when we initialize the interfaces.
            thisPort.BrakeControl.BrakeSetting(0, BRAKE_ALLOW_MOTION);
            thisPort.BrakeControl.BrakeSetting(1, BRAKE_ALLOW_MOTION);

            auto nodeCntOnPort = thisPort.NodeCount();
            m_nodeCount += nodeCntOnPort;
            // TODO check, not sure if those to_strings will be very readable
            buf = ("Port: " + to_string(thisPort.NetNumber()) + 
                ", State: " + to_string(thisPort.OpenState()) + 
                ", Node#" + to_string(nodeCntOnPort) + ".");
            logInfo(buf.c_str());
            // Iterate nodes on this port
            for (size_t nodeIndex = 0; nodeIndex < nodeCntOnPort; nodeIndex++) {
                string nodeType;
                switch (nodeIndex)
                {
                case 0:
                    nodeType = "forward_backward";
                case 1:
                    nodeType = "tilt";
                case 2:
                    nodeType = "aperture";
                default:
                    nodeType = "default";
                }

                Node wrappedNode = Node(thisPort.Nodes(nodeIndex), nodeType);

                m_nodes.push_back(std::reference_wrapper<Node>(wrappedNode));  //TODO: Push NodeWrapper here instead
            }
        }

        initializedCorrectly = true;
    }
    catch (mnErr& theErr) {
        // (defined by the mnErr class)
        buf = ("MotorAPI() constructor | addr: " + to_string( theErr.TheAddr) + 
            " | err: " + to_string(theErr.ErrorCode) + " | msg: " + theErr.ErrorMsg);
        logError(buf.c_str());
    }
}

MotorAPI::~MotorAPI(void) {
    logInfo("Teknic Shutting down. Disabling nodes, and closing port");
    for (size_t i = 0; i < m_nodeCount; i++) {
        auto thisNode = m_nodes[i].get();
        thisNode.disable();
    }
    m_manager->PortsClose();
    logInfo("Teknic Shutdown!");
}

int MotorAPI::home()
{
    if (!wasInitializedCorrectly())
        return -1;

    for (auto node : m_nodes) {
        node.get().home();
    }

    return 0;
}

int MotorAPI::retreat()
{
    if (!wasInitializedCorrectly())
        return -1;

    for (auto e : m_nodes) {
        e.get().retreat();
    }
    return 0;
}

int MotorAPI::move(std::vector<int> positions)
{
    if (!wasInitializedCorrectly())
        return -1;

    if (positions.size() > m_nodeCount)
        return -1;

    for (size_t i = 0; i < positions.size(); i++)
    {
        // the speed and acc variable are default
        m_nodes[i].get().moveHigh(positions[i], 2, 2);
    }
    return 0;
}

bool MotorAPI::wasInitializedCorrectly()
{
    return initializedCorrectly;
}

Convertor::Convertor(const double _min_level, const double _max_level, const double _max_motor_val)
{
    min_level = _min_level;
    max_level = _max_level;
    max_motor_val = _max_motor_val;
}

Convertor::~Convertor()
{
}

int Convertor::convert(const int val)
{
    if (val < min_level)
        return (int) round(min_level * max_motor_val / max_level);

    if (val > max_level)
        return (int)round(max_motor_val);

    return (int)round(val * max_motor_val / max_level);
}
