import sys
import json
from concurrent import futures
import threading
import os
import time

import grpc
import armMessages_pb2
import armMessages_pb2_grpc

from ui.kinovaArmGUI import Ui_armGui as armGUI
from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtCore import QObject, QThread, pyqtSignal

from link6Arm import KinovaArm
from log import setLogFileName, printLog


textRed = "ad1010"
textYellow = "d49a13"
textGreen = "205710"
textBlack = "000000"

monkey_path = "./monkeys.json"

class KinovaServer(QObject):
    '''
    ###############################################
    Init Functions, set up gui, connect to grpc
    ###############################################
    '''
    commandSignal = pyqtSignal(str)
    errorSignal = pyqtSignal(str)
    enableSignal = pyqtSignal(bool)
    progressSignal = pyqtSignal(int)
    GRPCserver = None
    def __init__(self, arm):
        super(KinovaServer, self).__init__()
        self.arm = arm
        self.movementLock = threading.Lock()
        self.movementEvent = threading.Event()
        self.gripperEvent = threading.Event()
        self.notifHandle1 = None
        self.notifHandle2 = None
        self.insanityCheck = 0
        self.isInFault = False

    def begin(self):
        self.initGui()

    def initGui(self):
        app = QApplication(sys.argv)
        win = QMainWindow()
        ui = armGUI()
        ui.setupUi(win)
        
        self.ui = ui
        self.setUpGUIConnections()
        
        win.show()
        
        def scoped():
            ret = app.exec_()
            if(self.arm.checkArmConnection()):
                if self.notifHandle1 is not None:
                    self.arm.unsubscribe(self.notifHandle1)
                if self.notifHandle2 is not None:
                    self.arm.unsubscribe(self.notifHandle2)
                self.arm.disconnectBase()
            #self.GRPCserver.stop()
            return ret
        
        sys.exit(scoped())
        return

    def armNotificationCallback(self, completed):
        printLog("In Arm Notif Callback")
        if completed:
            self.isInFault = False
        else:
            printLog("Action failed, likely due to singularity")
            self.isInFault = True
        self.movementEvent.set()
        return
    
    def gripperNotificationCallback(self, completed):
        printLog("In Gripper Notif Callback")
        if completed:
            self.isInFault = False
        else:
            printLog("FUCK")
            self.isInFault = True
        self.gripperEvent.set()
        return


    '''
    ###############################################
    Arm Functions: The actual meat of this code
    ###############################################
    '''

    def getArmStatus(self):
        ret, err = self.arm.getArmStatus()
        if ret < 0:
            errText = "Error in getting arm status: \n" + str(err)
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)
        return ret

    def activateGripper(self):
        ret, err = self.arm.activateGripper()
        if ret < 0:
            errText = "Error in activating the gripper: \n" + str(err)
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)
        return ret

    def moveArmHome(self, wait=False):
        printLog("Attempting to get movement lock")
        self.movementLock.acquire()
        printLog("Got movement lock")
        self.movementEvent.clear()
        printLog("Cleared event doing move")
        ret, err = self.arm.moveHome()
        if ret < 0:
            printLog("Got error in movement")
            self.movementLock.release()
            errText = "Error in moving the arm to home position: \n" + str(err)
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)
            return ret
        if(wait):
            printLog("Waiting for movement to finish")
            self.movementEvent.wait()
        printLog("Releasing Lock")
        self.movementLock.release()
        if self.isInFault:
            printLog("Fault has occured during movement")
            errText = ("Arm has encountered a problem during movement. Likely a" 
            "singularity. Faults need to be cleared to continue. \n"
            "<<----------- Use the clear button to clear faults")
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)

            ret = -16
        printLog("Done")
        return ret

    def openGripper(self, width, wait=False):
        printLog("Attempting to get movement lock")
        self.movementLock.acquire()
        self.gripperEvent.clear()
        printLog("Got movement lock")
        self.movementEvent.clear()
        printLog("Cleared event doing gripper move")
        ret, err = self.arm.moveGripper(width) 
        if ret < 0:
            errText = "Error in moving the gripper to a width: \n" + str(err)
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)

        if wait:
            printLog("Waiting for gripper event")
            self.gripperEvent.wait()
            printLog("Finished waiting for event")
        self.movementLock.release()
        printLog("Finished with openGripper")
        return ret

    def moveArmToPosition(self, x, y, z, theta, phi, chi, width):
        printLog("Attempting to get movement lock")
        self.movementLock.acquire()
        printLog("Got movement lock")
        self.movementEvent.clear()
        '''
        printLog("Cleared event doing gripper move")
        ret, err = self.arm.moveGripper(width) 
        if ret < 0:
            self.movementLock.release()
            errText = "Error in moving the gripper to a width: \n" + str(err)
            self.changeErrorBoxText(errText, 12, textRed)
            return ret
        printLog("Doing normal move")
        '''
        ret, err = self.arm.referencedMoveArmCartesian(x, y, z, theta, phi, chi)
        if ret < 0:
            self.movementLock.release()
            errText = "Error in moving the arm to a position: \n" + str(err)
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)
            return ret
        printLog("Waiting on event")
        self.movementEvent.wait()
        printLog("Event done, releasing lock")
        self.movementLock.release()
        if self.isInFault:
            printLog("Fault has occured during movement")
            errText = ("Arm has encountered a problem during movement. Likely a" 
            "singularity. Faults need to be cleared to continue. \n"
            "<<----------- Use the clear button to clear faults")
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)
            ret = -16
        return ret

    def emergencyStop(self):
        ret, err = self.arm.stop()
        if ret < 0:
            errText = "THIS IS BAD\nError in emergency stop: \n" + str(err)
            printLog(errText)
            self.changeErrorBoxText(errText, 18, textRed)
        return ret


    '''
    ###############################################
    GRPC FUNCTIONS, these functions must be named this
    ###############################################
    '''
    def armReady(self, request, context):
        printLog("Ready Request recieved")
        ret = self.arm.checkArmReady()
        if not ret:
            self.moveArmHome(wait = True)
            ret = self.arm.checkArmReady()
        if not ret:
            ret = -1
        else:
            ret = 1
        return armMessages_pb2.readyResponse(flag=ret)

    def armStatus(self, request, context):
        printLog("Status Request: ", request.flag)
        comString = "Req: status"
        self.addCommandToList(comString)
        ret = self.getArmStatus()
        return armMessages_pb2.statusResponse(flag=ret)
    
    def gripperOpen(self, request, context):
        printLog("Gripper open: ", request.width)
        comString = "Opn: {}".format(request.width)
        self.addCommandToList(comString)
        ret = self.openGripper(request.width, wait=True)
        return armMessages_pb2.moveResponse(responseCode=ret)

    def armControl(self, request, context):
        printLog("Arm Control: ", request.lateral, request.depth, request.height)
        comString = "Mov: {} {} {} :: {} {} {} :: {}".format(request.lateral, request.depth, request.height, request.theta, request.phi, request.chi, request.width)
        self.addCommandToList(comString)
        ret = self.moveArmToPosition(request.lateral, request.depth, request.height, request.theta, request.phi, request.chi, request.width)
        return armMessages_pb2.moveResponse(responseCode=ret)

    def armHome(self, request, context):
        printLog("Arm Home: ", request.flag)
        comString = "Mov: home"
        self.addCommandToList(comString)
        ret = self.moveArmHome(wait=True)
        return armMessages_pb2.moveResponse(responseCode=ret)
    
    def stopArm(self, request, context):
        ret = self.emergencyStop()
        printLog("STOP ARM message")
        comString = "STOP ARM"
        self.addCommandToList(comString)
        return armMessages_pb2.moveResponse(responseCode=ret)

    def armFeedback(self, request, context):
        #This is future direction where we output the torques of the arm as well, so this is nonFunctioning right now
        printLog("WHY THE FUCK ARE WE HERE")
        comString = "Req: forces"
        self.addCommandToList(comString)
        return armMessages_pb2.torqueResponse(joint1=1, joint2=2, joint3=3, joint4=4, joint5=5, joint6=6, endpoint=7)


    '''
    ###############################################
    FLUFF GUI FUNCTIONS, looks big but its all just fluff
    ###############################################
    '''

    def addCommandToList(self, item):
        self.commandSignal.emit(item)

    def changeErrorBoxText(self, text, size, color):
        fullText = "<span style=\" font-size:{}pt; font-weight:600; color:#{};\" >".format(size, color)
        fullText += text
        fullText += "</span>"
        self.errorSignal.emit(fullText)
    
    def clearErrorBox(self):
        if self.isInFault:
            ret, err = self.arm.clearFaults()
            if(ret < 0):
                errText = "ERROR WHILE CLEARING FAULTS REALLY BAD:\n " + str(err)
                printLog(errText)
                self.changeErrorBoxText(errText, 12, textRed)
                return
            time.sleep(.5)
            self.moveArmHome()
            self.isInFault = False
        self.ui.armStatusBox.setText("")

    def safeThreadedAddToList(self, text):
        self.ui.commandList.addItem(text)
    
    def safeThreadedchangeErrorBoxText(self, text):
        self.ui.armStatusBox.setText(text)

    def safeThreadedButtonEnable(self, val):
        self.ui.armButton.setEnabled(val)
        
    def setGrpcStatus(self, color):
        self.ui.grpcStatus.setStyleSheet("QProgressBar::chunk " +
                "{" +
                "background-color: {};".format(color) +
                "}")
        self.ui.grpcStatus.setValue = 100

    def setArmStatus(self, color):
        self.ui.armStatus.setStyleSheet("QProgressBar::chunk " +
                "{" +
                "background-color: {};".format(color) +
                "}")
        self.ui.armStatus.setValue = 100

    def changeIndicatorColor(self, color):
        self.ui.progressBar.setStyleSheet("QProgressBar::chunk {{background-color: {};}}".format(color))

    def setConnectProgress(self, val):
        self.ui.connectProgress.setValue(val)

    def setPowerProgress(self, val):
        self.ui.powerProgress.setValue(val)

    '''
    ###############################################
    Middle Ground functions, things that are GUI but meaty
    ###############################################
    '''
    
    def intermediateConnect(self):
        if(not self.arm.checkArmConnection()):
            ret, err = self.arm.connectToBase()
            if ret < 0:
                errText = "Error in connecting to the arm: \n" + str(err) 
                printLog(errText)
                self.changeErrorBoxText(errText, 12, textRed)
                self.setConnectProgress(0)
                return
            self.notifHandle1 = self.arm.addArmNotificationCallback(self.armNotificationCallback)
            self.notifHandle2 = self.arm.addGripperNotificationCallback(self.gripperNotificationCallback)
            self.setConnectProgress(100)
            self.ui.connectButton.setStyleSheet("background-color : green")
            if(self.arm.checkArmPowered()):
                self.ui.armButton.setStyleSheet("background-color : green")
                self.setPowerProgress(100)
        else:
            self.arm.unsubscribe(self.notifHandle1)
            self.arm.unsubscribe(self.notifHandle2)
            self.notifHandle1 = None
            self.notifHandle2 = None
            ret, err = self.arm.disconnectBase()
            if ret < 0:
                errText = "Error in disconnecting to the arm: \n" + str(err)
                printLog(errText) 
                self.changeErrorBoxText(errText, 12, textRed)
                return
            self.setConnectProgress(0)
            self.ui.connectButton.setStyleSheet("background-color : lightgray")
        return

    def intermediatePower(self):
        if(not self.arm.checkArmConnection()):
            errText = "Cannot turn arm on without connecting first!"
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)
            return
        if(self.arm.checkArmPowered()):
            #Arm is already on
            ret, err = self.arm.powerArmOff()
            if ret < 0:
                errText = "Error in turning off the arm: \n" + str(err)
                printLog(errText) 
                self.changeErrorBoxText(errText, 12, textRed)
                return
            self.setPowerProgress(0)
            self.ui.armButton.setStyleSheet("background-color : lightgray")

        else:
            #Arm is already off
            ret, err = self.arm.powerArmOn()
            if ret < 0:
                errText = "Error in turning on the arm: \n" + str(err)
                printLog(errText) 
                self.changeErrorBoxText(errText, 12, textRed)
                return
            def waitForPower():
                printLog("Started the async shit")
                self.enableSignal.emit(False)
                self.ui.armButton.setStyleSheet("background-color : lightblue")
                printLog("Begun the crash shit")
                for ret, err in self.arm.waitForPowerOn():
                    printLog("in loop: ", ret)
                    if ret < 0:
                        errText = "Error in turning on the arm: \n" + str(err)
                        printLog(errText)
                        self.changeErrorBoxText(errText, 12, textRed)
                        self.enableSignal.emit(True)
                        return
                    self.progressSignal.emit(ret)
                    printLog("Bottom Loop")
                printLog("out of loop")
                self.ui.armButton.setStyleSheet("background-color : green")
                self.enableSignal.emit(True)
            #waitForPower()
            #t = threading.Thread(target = waitForPower)
            #t.start()
        return
    
    def checkPower(self):
        if(self.arm.checkArmPowered()):
            self.ui.armButton.setStyleSheet("background-color : green")
            self.setPowerProgress(100)

    def text_changed(self, s):
        side = self.mDB[s]["side"]
        distance = self.mDB[s]["distance"]
        mult = 1
        try:
            assert (type(distance) == float or type(distance) == int), "The type of distance is not a numeric, check monkeys.json for {}".format(s)

            if side == 'left':
                mult = -1
            self.shoulder_offset = mult * distance
        except Exception as e:
            errText = "Unable to load the file selected, likely the wrong format: \n" + str(e)
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)

    def developerInput(self):
        if self.safteyCount < 2:
            self.safteyCount += 1
            return
        self.safteyCount = 0
        printLog("DEVELOPER MODE, enter command, enter \"cancel\" to cancel")
        code = input()
        if code == "cancel":
            return
        try:
            eval(code)
        except Exception as e:
            printLog(e)
            errText = "JESUS FUCK WHAT DID YOU DO IN DEV MODE: \n" + str(e)
            printLog(errText)
            self.changeErrorBoxText(errText, 12, textRed)
        return
        
    def setUpGUIConnections(self):
        self.commandSignal.connect(self.safeThreadedAddToList)
        self.errorSignal.connect(self.safeThreadedchangeErrorBoxText)
        self.enableSignal.connect(self.safeThreadedButtonEnable)
        self.progressSignal.connect(self.setPowerProgress)

        self.ui.clearButton.clicked.connect(self.clearErrorBox)
        self.ui.stopButton.clicked.connect(self.emergencyStop)
        self.ui.connectButton.clicked.connect(self.intermediateConnect)
        self.ui.armButton.clicked.connect(self.intermediatePower)
        self.ui.gripperButton.clicked.connect(self.activateGripper)
        self.ui.homeButton.clicked.connect(self.moveArmHome)
        self.ui.devButton.clicked.connect(self.developerInput)
        self.ui.checkPowerButton.clicked.connect(self.checkPower)

        self.safteyCount = 0
        '''
        #Old code from when monkey selection would be here and not in trial creation
        F = open(monkey_path)
        mDB = json.load(F)
        F.close()
        self.mDB = mDB
        for m in mDB:
            self.ui.monkeySelectBox.addItem(m)
        self.ui.monkeySelectBox.currentTextChanged.connect(self.text_changed)
        '''


class TestServer:
    def __init__(self):
        printLog("In init")
    def additionalshit(self):
        return
    def additionalshit1(self):
        return
    def additionalshit2(self):
        return
    def additionalshit3(self):
        return
    def armStatus(self, request, context):
        printLog("Status Request: ", request.flag)
        return armMessages_pb2.statusResponse(flag=13)

    def armControl(self, request, context):
        printLog("Arm Control: ", request.X, request.Y, request.Z)
        return armMessages_pb2.moveResponse(responseCode=6)

    def armHome(self, request, context):
        printLog("Arm Home: ", request.flag)
        return armMessages_pb2.moveResponse(responseCode=7)

    def armFeedback(self, request, context):
        printLog("WHY THE FUCK ARE WE HERE")
        return armMessages_pb2.torqueResponse(joint1=1, joint2=2, joint3=3, joint4=4, joint5=5, joint6=6, endpoint=7)


def serve(service=None):
    if service == None:
        service = TestServer()

    server = grpc.server(futures.ThreadPoolExecutor(max_workers=5))
    armMessages_pb2_grpc.add_armCommunicationServicer_to_server(service, server)
    server.add_insecure_port('[::]:50051')
    server.start()
    printLog("GRPC Server started")
    kService.GRPCserver = server



if __name__ == "__main__":
    os.system("title KinovaServer")
    setLogFileName()
    arm = KinovaArm()
    kService = KinovaServer(arm)
    #serve()
    serve(kService)
    kService.begin()