from kortex_api.autogen.client_stubs.SessionClientRpc import SessionClient
from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient

from kortex_api.RouterClient import RouterClient
from kortex_api.UDPTransport import UDPTransport
from kortex_api.SessionManager import SessionManager

from kortex_api.autogen.messages import Session_pb2, Base_pb2, ProgramRunner_pb2, Common_pb2, PluginManager_pb2, Plugin_pb2
from kortex_api.autogen.client_stubs.PluginClientRpc import PluginClient
from kortex_api.autogen.client_stubs.PluginManagerClientRpc import PluginManagerClient
from kortex_api.MqttTransport import MqttTransport

from kortex_api.autogen.client_stubs.BaseCyclicClientRpc import BaseCyclicClient
from kortex_api.autogen.messages.Common_pb2 import ModeSelection,OperatingModeType, CartesianReferenceFrame
from kortex_api.exceptions.KServerException import KServerException

from jsonschema import validate
import json

import time
import numpy as np
from google.protobuf import json_format

from log import printLog

from PyQt5.QtCore import QObject

waypoint_count = 1
#TODO add fault clearing capability
#TODO maybe have a flexible gripper plugin

class KinovaArm(QObject):
    '''
    This is the class that is for connecting and using the Kinova link6 arm.
    It has been designed to have commands for placing the arm in any position 
    within its range (theoretically). It manages the connections and error 
    returns, but not fault clearing. 
    While this class was built to be used within the kinovaArmServer class/gui,
    it has enough general function to be used outside of it. However there may 
    be some confusing internal functions that serve no purpose if not being used
    with the GUI.
    For some of the short and very simple functions there will be minimal 
    comments as the name of the function should be explanation enough. Other 
    functions will have both a top comment and in line comments.
    Almost all external facing functions (functions to be called by a user or
    other controller) have a standardized return format:
    (return_code, error)
    if return_code < 0 then an error has occured but has been avoided with a 
    try: statement, the raised error is then the value of error.
    All negative return codes are unique so they can also be used to trace 
    errors in outside programs
    if return_code > 0 then the function has worked successfully, not all 
    positive return codes are unique as not all are "codes" in the traditional 
    sense, and if there is no special meaning behind the value it defaults to 1
    -PMJ
    '''
    CONTROLLER_ADDRESS = "169.254.131.10" #This can be set relatively arbitrarily by either the kinova webApp or tablet
    MQTT_PORT = 1883
    UDP_PORT = 10001

    actionStarted = False
    #Home position coordinates in both forms
    homeCart = [0, -.200, .480, 90, -180, 0]
    homeAngular = [-72.9454651,
                    -50.2982483,
                    -118.821823,
                    -41.3727722,
                    117.561745,
                    -141.523972]
    xLimits = [-.381, .381]
    yLimits = [-.200, -.600]
    zLimits = [.380, .580]
    homeSet = False

    def __init__(self):
        super(KinovaArm, self).__init__()
        return
    
    def connectToBase(self):
        '''
        This function is pretty much straight ripped from the Kinova code 
        examples. We connect to the base and initialize our gripper client
        '''
        try:
            session_info = Session_pb2.CreateSessionInfo()
            session_info.username = "admin"
            session_info.password = "admin"
            session_info.session_inactivity_timeout = 60000 # (milliseconds)
            session_info.connection_inactivity_timeout = 2000 # (milliseconds)

            self.transport = MqttTransport()
            errorCallback = lambda kException: printLog("____CallBack Error____: {}".format(kException))
            
            router = RouterClient(self.transport, errorCallback)
            self.transport.connect(self.CONTROLLER_ADDRESS, self.MQTT_PORT)

            printLog("Creating Session")
            self.session_client = SessionClient(router)
            self.session_client.CreateSession(session_info)
            printLog("Session Created")

            self.base = BaseClient(router)
            printLog("Connected to the Base")
            
            plugin_name = "robotiq_plugin"
            
            plugin_manager = PluginManagerClient(router)
            plugin_list = plugin_manager.GetPluginsList()
            for plugin in plugin_list.plugin_info_list:
                if plugin.handle.identifier == plugin_name:
                    printLog(plugin.handle.identifier)
                    printLog(plugin.plugin_state.state)
            
            self.gripper_plugin = PluginClient(router, plugin_name)
            printLog("Connected to gripper plugin")
            
            return (1, None)
            
        except Exception as e:
            printLog("Failed to connect to base; Error: ", e)
            return (-1, e)

    def disconnectBase(self):
        try:
            self.session_client.CloseSession()
            self.transport.disconnect()
            printLog("Disconnected from base")
            return (1, None)
        except Exception as e:
            return (-2, e)
            
        
    def checkArmConnection(self):
        '''
        This is one of the few external functions that doesn't conform to 
        (code, err) as it will never have a good error.
        Because the base can be disconnect by connecting on the webapp or 
        other causes, this isn't as simple as setting a flag upon connect
        '''
        try:
            self.base.GetArmState()
            return True
        except:
            return False
        
    def getArmStatus(self):
        try:
            return (self.base.GetArmState().active_state, None)
        except Exception as e:
            return (-1, e)
        

    def checkArmPowered(self):
        #Armstate_idle is the state of a conencted but unpowered arm. 
        try:
            return(self.base.GetArmState().active_state != Base_pb2.ARMSTATE_IDLE)
        except:
            return False
        
    def powerArmOn(self):
        try:
            printLog("Attmepting to turn the arm on")
            self.base.ActivateRobot()
            printLog("Turned on Arm, may take a minute")
            return(1, None)
        except Exception as e:
            return(-10, e)
                
    def powerArmOff(self):
        try:
            printLog("Turning arm off")
            self.base.DeactivateRobot()
            return(1,None)
        except Exception as e:
            return (-11, e)
    
    def waitForPowerOn(self, timeout=60):
        '''
        Turning the arm on is an ASYNC operation so this function will wait for
        it to finish. We do it as a generator function so that we can give
        live updates to whatever has called this function.
        Here I have decided to make the positive values as a rough percentage
        of how done this process is. This is so it can adjust the GUIs progress
        bar.
        Because turning the arm on is ASYNC, it could theoretically take forever
        because of some failure, here timeout represents how long we will wait 
        for it to turn on before thinking something is wrong. It generally takes
        20-30 seconds so 60 is not as conservative as it may appear
        '''
        try:
            secs = 0
            state = self.base.GetArmState()
            printLog("Started Wait function")
            while state.active_state != Base_pb2.ARMSTATE_ARM_OPERATIONAL:
                printLog("Top of loop")
                if secs > timeout:
                    printLog("Timed Out")
                    yield (-12, "Failed to turn arm on within the timeout window (it may still yet turn on if timeout was short)")
                    return
                if(state.active_state == Base_pb2.ARMSTATE_INITIALIZATION):
                    printLog("Yeilding 30+")
                    yield (30 + secs, None)
                elif(state.active_state == Base_pb2.ARMSTATE_BRAKE_RELEASING):
                    printLog("Yeilding 90")
                    yield(90, None)
                
                printLog("Before Sleep")
                #Doing Psuedo Sleep
                tic = time.time()
                while(time.time() - tic < 1.0):
                    continue
                #time.sleep(1)
                printLog("Post Sleep")
                secs += 1
                state = self.base.GetArmState()
                printLog("Bottom Loop")
            printLog("Out of Loop")
            yield (100, None)
            return
        except Exception as e:
            yield(-13, e)

    def checkArmReady(self):
        return self.homeSet
    
    def clearFaults(self):
        try:
            self.base.ClearFaults()
            return (1, None)
        except Exception as e:
            return(-14, e)
        
    def addArmNotificationCallback(self, func):
        '''
        Func: an external function that will be called when an action ends.
        I use this to allow for waiting on actions to finish as func sets
        a threading.Event object.
        Theres also a weird bit where the actions always start with an actionEnd
        action even before the start action which is weird, so we have to set a
        flag once the action_started thing happens
        '''
        def logicWrapper(data):
            data = json_format.MessageToDict(data)
            if data["actionEvent"] == "ACTION_FEEDBACK":
                return
            if data["actionEvent"].strip(" ") == "ACTION_END":
                if not self.actionStarted:
                    return
                printLog("Got inside correctly, calling func")
                self.actionStarted = False
                func(True)
            elif data["actionEvent"].strip(" ") == "ACTION_START":
                printLog("Setting started")
                self.actionStarted = True
                return
            elif data["actionEvent"].strip(" ") == "ACTION_ABORT":
                self.actionStarted = False
                printLog("In action abort, either a stop or a fault")
                st = None
                try:
                    #self.connectToBase()
                    #st = self.base.GetArmState().active_state
                    pliasd = 1
                except Exception as e:
                    printLog("Some shit is real fucked, failed to get base state")
                    printLog(e)
                    func(False)
                    return
                if st == Base_pb2.ARMSTATE_ARM_OPERATIONAL or True:
                    printLog("Action Abort was just a stop")
                    func(True)
                else:
                    printLog("Action Abort was a fault")
                    func(False)

            else:
                printLog("Got a different callback function: ", data["actionEvent"])
                return
        self.base.OnNotificationActionTopic(logicWrapper, Base_pb2.NotificationOptions())
        return
    
    def addGripperNotificationCallback(self, func):
        
        def logicWrapper(data):
            data = json_format.MessageToDict(data)
            if data["actionEvent"].strip(" ") == "ACTION_END":
                printLog("Gripper action finished")
                func(True)
            elif data["actionEvent"].strip(" ") == "ACTION_ABORT":
                printLog("In Gripper Action Abort")
                if self.gripper_plugin.GetStatus().state == Plugin_pb2.STATE_ERROR:
                    printLog("Gripper action abort was an actual error")   
                    func(False)
                else:
                    printLog("Gripper Action abort was likely just a stop")
                    func(True)
            else:
                printLog("Got a different callback function: ", data["actionEvent"])
                return
            return
        self.gripper_plugin.OnNotificationActionTopic(logicWrapper, Plugin_pb2.NotificationOptions())
        return
    
    def unsubscribe(self, notif_handle):
        if notif_handle is None:
            return
        try:
            self.base.Unsubscribe(notif_handle)
        except:
            printLog("Failed to unsub from notif handle")
        return
    
    def change_operating_mode(self, operating_mode_type : str):
        '''
        This is an internal function that is straight from the kinova example
        code. One needs to change operating modes to do different things
        with the arm. We will almost alwasy be in auto or stop
        '''
        # OPERATING_MODE_UNSPECIFIED (0):       Unspecified operating mode
        # OPERATING_MODE_JOG_MANUAL (1):        Jog manual operating mode
        # OPERATING_MODE_HAND_GUIDING (2):      Hand guiding operating mode
        # OPERATING_MODE_HOLD_TO_RUN (3):       Hold to run operating mode
        # OPERATING_MODE_AUTO (4):              Automatic operating mode
        # OPERATING_MODE_MONITORED_STOP (5):    Monitored stop operating mode

        mode = ModeSelection()
        mode.operating_mode = OperatingModeType.Value(operating_mode_type)
        self.base.SelectOperatingMode(mode)
        return
    
    def stop(self):
        '''
        Not quite a full emergency stop but pretty close.
        We should also have a button that provides full emergency stop features.
        '''
        try:
            self.change_operating_mode("OPERATING_MODE_MONITORED_STOP")
            return (1, None)
        except Exception as e:
            return (-3, e)
    
    def activateGripper(self):
        activate_input = {}
        action_name = "Activate"
        return self.pluginCommand(action_name, activate_input)
    
    def moveGripper(self, width, velocity=64, force=10):
        maxWidth = 42
        minWidth = 0
        maxVal = 243 #255 is actually fully closed but the math is easier this way

        if(width > maxWidth):
            widthVal = 0
        elif(width < minWidth):
            widthVal = maxVal
        else:
            widthVal = maxVal - int(width * maxVal / (maxWidth - minWidth))
        
        move_input = {
            "position":widthVal,
            "velocity":velocity,
            "force":force
        }
        action_name = "Move"
        return self.pluginCommand(action_name, move_input)
        
    def pluginCommand(self, action_name, plugin_input):
        '''
        A purely internal function that activateGripper and moveGripper call.
        This function searches all gripper plugin actions, finds the one we
        called and passes our input to it. This cut down on lots of duplicated 
        lines.
        '''
        try:
            action_list = self.gripper_plugin.GetActionTypes()

            for available_action in action_list.actions:
                if available_action.friendly_name == action_name:
                    action = Plugin_pb2.Action()
                    action.serialization_type= Plugin_pb2.DataType.CONFIGURATION_TYPE_JSON
                    action.handle.CopyFrom(available_action.handle)
                    validate(plugin_input, json.loads(available_action.input_schema))
                    action.input = json.dumps(plugin_input)
                    self.gripper_plugin.StartAction(action)
                    return (1, None)
            return (-4, "Unable to find gripper action, but able to see list of actions")
        except Exception as e:
            return (-5, e)
        
    def translateAngles(self, theta, chi, phi):
        #TODO change this to actually calculate the hard shit with K2
        dx = self.homeCart[3]
        dy = theta + self.homeCart[4]
        dz = self.homeCart[5]
        return dx, dy, dz

    def translatePosition(self, lateral, depth, height):
        X = self.homeCart[0] + lateral
        Y = self.homeCart[1] - depth
        Z = self.homeCart[2] + height
        return X, Y, Z

        
    def moveHome(self):
        ret, err = self.angularMoveArm(*self.homeAngular)
        if not self.homeSet and ret >= 0:
            self.homeSet = True
            mp = self.base.GetMeasuredCartesianPose()
            self.homeCart[3] = round(mp.theta_x / 90) * 90
            self.homeCart[4] = round(mp.theta_y / 90) * 90
            self.homeCart[5] = round(mp.theta_z / 90) * 90
        return (ret, err)
    
    def referencedMoveArmCartesian(self, lateral, depth, height, theta, phi, chi, dur = 0):
        dx, dy, dz = self.translateAngles(theta, chi, phi)
        return(self.cartesianMoveArm(self.homeCart[0] + lateral,
                                    self.homeCart[1] - depth, #Arm is centered so that -y is forward
                                    self.homeCart[2] + height,
                                     dx, dy, dz, dur))
    
    def cartesianMoveArm(self, X, Y, Z, xo, yo, zo, dur=0):
        if not self.xLimits[0] <= X <= self.xLimits[1]:
            return (-14, "Commanded X position is not within the box we are operating in, X={}".format(X))
        if not self.yLimits[1] <= Y <= self.yLimits[0]:
            return (-14, "Commanded Y position is not within the box we are operating in, Y={}".format(Y))
        if not self.zLimits[0] <= Z <= self.zLimits[1]:
            return (-14, "Commanded Z position is not within the box we are operating in, Z={}".format(Z))
        if (X ** 2 + Y ** 2 + Z ** 2)**.5 > .925:
            return(-15, "Commanded position is fully outside of the arms range")
        try:
            R = 0
            self.change_operating_mode("OPERATING_MODE_AUTO")

            waypointB = Base_pb2.CartesianWaypoint()
            waypointB.pose.x = X            # in meters
            waypointB.pose.y = Y            # in meters
            waypointB.pose.z = Z            # in meters
            waypointB.blending_radius = R   # in meters
            waypointB.pose.theta_x = xo      # in degrees
            waypointB.pose.theta_y = yo      # in degrees
            waypointB.pose.theta_z = zo      # in degrees

            waypointB.reference_frame = CartesianReferenceFrame.Value("CARTESIAN_REFERENCE_FRAME_BASE")
            #Why the fuck is there all this redundancy here?

            waypointsDefinition = ((X, Y, Z, R, xo, yo, zo))
            waypointCount = len(waypointsDefinition)

            wptlist = Base_pb2.WaypointList()
            wptlist.use_optimal_blending = True


            array_wpts = np.array([])
            index = 0
            for i in range(0,waypointCount):

                np.append(array_wpts, waypointsDefinition[i])
                waypoint = wptlist.waypoints.add()
                waypoint.name = "waypoint_" + str(index)
                waypoint.cartesian_waypoint.CopyFrom(waypointB)
                index = index + 1

            #Add waypoints to waypoint list
            wptlist.waypoints.MergeFrom(array_wpts)
            wptlist.duration = dur # in seconds
            result = self.base.ValidateWaypointList(wptlist)

            if len(result.trajectory_error_report.trajectory_error_elements) == 0:
                printLog("Reaching cartesian pose trajectory...")
                self.base.ExecuteWaypointTrajectory(wptlist)

            else:
                printLog("Error found in trajectory")
                printLog(result.trajectory_error_report)
                return (-6, result.trajectory_error_report)
            return (1, None)
        except Exception as e:
            return (-7, e)
            
        
    def angularMoveArm(self, j1, j2, j3, j4, j5, j6):
        try:
            self.change_operating_mode("OPERATING_MODE_AUTO")

            waypointB = Base_pb2.AngularWaypoint()
            waypointB.angles.MergeFrom([j1, j2, j3, j4, j5, j6])
            waypointB.duration = 0
            waypointB.blending = 0

            waypointsDefinition = (([j1, j2, j3, j4, j5, j6], 0, 1))
            waypointCount = len(waypointsDefinition)

            wptlist = Base_pb2.WaypointList()
            wptlist.use_optimal_blending = True

            # Here we create waypoints from the waypointsDefinition array with a for loop
            array_wpts = np.array([])
            index = 0
            for i in range(0,waypoint_count):

                np.append(array_wpts, waypointsDefinition[i])
                waypoint = wptlist.waypoints.add()
                waypoint.name = "waypoint_" + str(index)
                waypoint.angular_waypoint.CopyFrom(waypointB)
                index = index + 1

            # Add waypoints to waypoint list
            wptlist.waypoints.MergeFrom(array_wpts)
            wptlist.duration = 0 # in seconds
            result = self.base.ValidateWaypointList(wptlist)

            if len(result.trajectory_error_report.trajectory_error_elements) == 0:
                printLog("Reaching angular pose trajectory...")
                self.base.ExecuteWaypointTrajectory(wptlist)
            else:
                printLog("Error found in trajectory")
                printLog(result.trajectory_error_report)
                return (-8, result.trajectory_error_report)
            return (1, None)
        except Exception as e:
            return (-9, e)
    
    def inverseKinematics(self, lateral, depth, height, theta, chi, phi, guess=[-72.9454651, -50.2982483,
                    -118.821823, -41.3727722, 117.561745, -141.523972]):
        try:
            input_joint_angles = self.base.GetMeasuredJointAngles()
            pose = self.base.GetMeasuredCartesianPose()
        except KServerException as ex:
            printLog("Unable to get current robot pose")
            printLog(
                "Error_code:{} , Sub_error_code:{} ".format(
                    ex.get_error_code(), ex.get_error_sub_code()
                )
            )
            printLog("Caught expected error: {}".format(ex))
            return None

        X, Y, Z = self.translatePosition(lateral, depth, height)
        tX, tY, tZ = self.translateAngles(theta, chi, phi)
        
        input_IkData = Base_pb2.IKData()
        # Fill the IKData Object with the cartesian coordinates that need to be converted
        input_IkData.cartesian_pose.x = X
        input_IkData.cartesian_pose.y = Y
        input_IkData.cartesian_pose.z = Z
        input_IkData.cartesian_pose.theta_x = tX
        input_IkData.cartesian_pose.theta_y = tY
        input_IkData.cartesian_pose.theta_z = tZ

        # Fill the IKData Object with the guessed joint angles
        for i, joint_angle in enumerate(input_joint_angles.joint_angles):
            jAngle = input_IkData.guess.joint_angles.add()
            # '- 1' to generate an actual "guess" for current joint angles
            jAngle.value = joint_angle.value - 10
        try:
            #printLog("Computing Inverse Kinematics using joint angles and pose...")
            computed_joint_angles = self.base.ComputeInverseKinematics(input_IkData)
        except KServerException as ex:
            #printLog("Unable to compute inverse kinematics")
            '''
            printLog(
                "Error_code:{} , Sub_error_code:{} ".format(
                    ex.get_error_code(), ex.get_error_sub_code()
                )
            )
            printLog("Caught expected error: {}".format(ex))
            '''
            return None
        '''
        printLog("Joint ID : Joint Angle")
        joint_identifier = 0
        for joint_angle in computed_joint_angles.joint_angles:
            printLog(joint_identifier, " : ", joint_angle.value)
            joint_identifier += 1
        '''
        return computed_joint_angles