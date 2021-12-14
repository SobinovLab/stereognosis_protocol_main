/*
* This Class is based on the code by PJ Naughter. The original code can be found here:
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
#include "stdafx.h"
#include "SerialEnumserParts.h"

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "advapi32.lib")

using namespace std;

bool SerialEnumserParts::QueryUsingSetupAPI(std::vector<std::pair<UINT, std::string>>& ports)
{
    GUID guid = GUID_DEVINTERFACE_COMPORT;
    DWORD dwFlags = DIGCF_PRESENT | DIGCF_DEVICEINTERFACE;

    //Create a "device information set" for the specified GUID
    HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&guid, nullptr, nullptr, dwFlags);
    if (hDevInfoSet == INVALID_HANDLE_VALUE)
        return false;

    //Finally do the enumeration
    bool bMoreItems = true;
    int nIndex = 0;
    SP_DEVINFO_DATA devInfo{};
    while (bMoreItems)
    {
        //Enumerate the current device
        devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
        bMoreItems = SetupDiEnumDeviceInfo(hDevInfoSet, nIndex, &devInfo);
        if (bMoreItems)
        {
            //Did we find a serial port for this device
            bool bAdded = false;

            std::pair<UINT, string> cpair;

            //Get the registry key which stores the ports settings
            ATL::CRegKey deviceKey;
            deviceKey.Attach(SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE));
            if (deviceKey != INVALID_HANDLE_VALUE)
            {
                int nPort = 0;
#pragma warning(suppress: 26486)
                if (QueryRegistryPortName(deviceKey, nPort))
                {
                    cpair.first = nPort;
                    bAdded = true;
                }
            }

            //If the port was a serial port, then also try to get its friendly name
            if (bAdded)
            {
#pragma warning(suppress: 26489)
                if (QueryDeviceDescription(hDevInfoSet, devInfo, cpair.second)) {
#pragma warning(suppress: 26489)
                    ports.push_back(cpair);
                }
            }
        }

        ++nIndex;
    }

    //Free up the "device information set" now that we are finished with it
    SetupDiDestroyDeviceInfoList(hDevInfoSet);

    //Return the success indicator
    return true;
}

bool SerialEnumserParts::QueryRegistryPortName(ATL::CRegKey& deviceKey, int& nPort)
{
    //What will be the return value from the method (assume the worst)
    bool bAdded = false;

    //Read in the name of the port
    string sPortName;
    if (RegQueryValueString(deviceKey, _T("PortName"), sPortName))
    {
        //If it looks like "COMX" then
        //add it to the array which will be returned
        const size_t nLen = sPortName.length();
        if (nLen > 3)
        {
#pragma warning(suppress: 26481)
            if ((_tcsnicmp(sPortName.c_str(), _T("COM"), 3) == 0) && IsNumeric((sPortName.c_str() + 3), false))
            {
                //Work out the port number
#pragma warning(suppress: 26481)
                nPort = _ttoi(sPortName.c_str() + 3);
                bAdded = true;
            }
        }
    }

    return bAdded;
}

bool SerialEnumserParts::RegQueryValueString(ATL::CRegKey& key, LPCTSTR lpValueName, string& sValue)
{
    //Reset the output parameter
    sValue.clear();

    //First query for the size of the registry value
    ULONG nChars = 0;
    LSTATUS nStatus = key.QueryStringValue(lpValueName, nullptr, &nChars);
    if (nStatus != ERROR_SUCCESS)
    {
        SetLastError(nStatus);
        return false;
    }

    //Allocate enough bytes for the return value
#pragma warning(suppress: 26472 26489)
    sValue.resize(static_cast<size_t>(nChars) + 1); //+1 is to allow us to null terminate the data if required
    const DWORD dwAllocatedSize = ((nChars + 1) * sizeof(TCHAR));

    //We will use RegQueryValueEx directly here because ATL::CRegKey::QueryStringValue does not handle non-null terminated data
    DWORD dwType = 0;
    ULONG nBytes = dwAllocatedSize;
#pragma warning(suppress: 26446 26489 26490)
    //nStatus = RegQueryValueEx(key, lpValueName, nullptr, &dwType, reinterpret_cast<LPBYTE>(sValue.data()), &nBytes);
    nStatus = RegQueryValueEx(key, lpValueName, nullptr, &dwType, (LPBYTE)(sValue.data()), &nBytes);
    if (nStatus != ERROR_SUCCESS)
    {
        SetLastError(nStatus);
        return false;
    }
    if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
    {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }
    if ((nBytes % sizeof(TCHAR)) != 0)
    {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }
#pragma warning(suppress: 26446 26489)
    if (sValue[(nBytes / sizeof(TCHAR)) - 1] != _T('\0'))
    {
        //Forcibly null terminate the data ourselves
#pragma warning(suppress: 26446 26489)
        sValue[(nBytes / sizeof(TCHAR))] = _T('\0');
    }

    return true;
}

bool SerialEnumserParts::QueryDeviceDescription(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA& devInfo, string& sFriendlyName)
{
    DWORD dwType = 0;
    DWORD dwSize = 0;
    //Query initially to get the buffer size required
    if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType, nullptr, 0, &dwSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return false;
    }
    std::vector<BYTE> friendlyName;
    friendlyName.resize(dwSize);
#pragma warning(suppress: 26446 26490)
    if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType, friendlyName.data(), dwSize, &dwSize))
        return false;
    if (dwType != REG_SZ)
    {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }
#pragma warning(suppress: 26490)
    sFriendlyName = reinterpret_cast<const TCHAR*>(friendlyName.data());
    return true;
}

bool SerialEnumserParts::IsNumeric(LPCSTR pszString, bool bIgnoreColon)
{
    const size_t nLen = strlen(pszString);
    if (nLen == 0)
        return false;

    //What will be the return value from this function (assume the best)
    bool bNumeric = true;

    for (size_t i = 0; i < nLen && bNumeric; i++)
    {
#pragma warning(suppress: 26481)
        if (bIgnoreColon && (pszString[i] == ':'))
            bNumeric = true;
        else
#pragma warning(suppress: 26472 26481)
            bNumeric = (isdigit(static_cast<int>(pszString[i])) != 0);
    }

    return bNumeric;
}
