#pragma once
#include <string>


std::string checkKinovaErrCode(int code)
{
    std::string msg;
    switch(code)
    {
        case 1:
            msg = "All good, no error";
            break;
        case -1:
            msg = "Arm Unable to connect to base";
            break;
        case -2:
            msg = "Arm unable to disconnect from base";
            break;
        case -3:
            msg = "Arm failed in emergency stop scenario";
            break;
        case -4:
            msg = "Arm failed to find gripper action, but was able to fully search list";
            break;
        case -5:
            msg = "Arm encountered error in finding and searching list of actions";
            break;
        case -6:
            msg = "Arm failed in setting up a cartesian move command";
            break;
        case -7:
            msg = "Arm failed in executing up a cartesian move command";
            break;
        case -8:
            msg = "Arm failed in setting up an angular move command";
            break;
        case -9:
            msg = "Arm failed in executing an angular move command";
            break;
        case -10:
            msg = "Arm hit exception while turning on";
            break;
        case -11:
            msg = "Arm hit exception while turning off";
            break;
        case -12:
            msg = "Arm timed out while turning on";
            break;
        case -13:
            msg = "Arm hit exception while turning on";
            break;
        case -14:
            msg = "One of the coordinates is outside of the designated monkey box range";
            break;
        case -15:
            msg = "The commanded coordinate is outside of the arms entire range";
            break;
        case -16:
            msg = "The arm encountered a problem in movement (likely a singularity) and had to abort";
        case -20:
            msg = "GRPC failed in making a request to the server";
            break;
        default:
            msg = "Encountered an Error number unplanned for";
            break;

    }
    return msg;

}