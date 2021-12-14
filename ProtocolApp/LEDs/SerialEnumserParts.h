#pragma once
/*
* This Class is based on the code by PJ Naughter. The original code can be found here:
* http://www.naughter.com/enumser.html
* And discussion of the methods in
* https://stackoverflow.com/questions/1388871/how-do-i-get-a-list-of-available-serial-ports-in-win32
* 
Module : enumser.h
Purpose: Defines the interface for a class to enumerate the serial ports installed on a PC
         using a number of different approaches

Copyright (c) 1998 - 2021 by PJ Naughter (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 

*/
#include <windows.h>
#include <string>
#include <vector>

//Other includes
#include <setupapi.h>
#include <winioctl.h>


class SerialEnumserParts
{
public:
    static bool QueryUsingSetupAPI(std::vector<std::pair<UINT, std::string>> &ports);
    static bool QueryRegistryPortName(ATL::CRegKey& deviceKey, int& nPort);
    static bool RegQueryValueString(ATL::CRegKey& key, LPCTSTR lpValueName, std::string& sValue);
    static bool QueryDeviceDescription(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA& devInfo, std::string& sFriendlyName);
    static bool IsNumeric(LPCSTR pszString, bool bIgnoreColon);
};

