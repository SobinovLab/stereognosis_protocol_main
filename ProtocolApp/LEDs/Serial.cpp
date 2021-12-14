#include "stdafx.h"
#include "Serial.h"

using namespace std;

Serial::Serial(const std::string portName, const std::string comPortFriendlyName)
{
    //We're not yet connected
    this->connected = false;

    string buf;
    string locPortName = portName;
    if (locPortName.length() == 0) {
        locPortName = findComPort(comPortFriendlyName);
        if (locPortName.length() == 0)
            return;
    }

    //Try to connect to the given port throuh CreateFile
    this->hSerial = CreateFile(locPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    //Check if the connection was successfull
    if (this->hSerial == INVALID_HANDLE_VALUE)
    {
        //If not success full display an Error
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {

            //Print Error if neccessary
            buf = "Serial: Handle was not attached. Reason: " + std::string(portName) + " not available.";
            logError(buf.c_str());
        }
        else
        {
            logError("Serial: Unknown error.");
        }
    }
    else
    {
        //If connected we try to set the comm parameters
        DCB dcbSerialParams = { 0 };

        //Try to get the current
        if (!GetCommState(this->hSerial, &dcbSerialParams))
        {
            //If impossible, show an error
            logError("Serial: Failed to get current serial parameters!");
        }
        else
        {
            //Define serial connection parameters for the arduino board
            dcbSerialParams.BaudRate = CBR_9600;
            dcbSerialParams.ByteSize = 8;
            dcbSerialParams.StopBits = ONESTOPBIT;
            dcbSerialParams.Parity = NOPARITY;
            //Setting the DTR to Control_Enable ensures that the Arduino is properly
            //reset upon establishing a connection
            dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

            //Set the parameters and check for their proper application
            if (!SetCommState(hSerial, &dcbSerialParams))
            {
                logWarning("Serial: Could not set Serial Port parameters");
            }
            else
            {
                //If everything went fine we're connected
                this->connected = true;
                //Flush any remaining characters in the buffers 
                PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
                //We wait 2s as the arduino board will be reseting
                Sleep(arduino_wait_time);
            }
        }
    }

}

Serial::~Serial()
{
    //Check if we are connected before trying to disconnect
    if (this->connected)
    {
        //We're no longer connected
        this->connected = false;
        //Close the serial handler
        CloseHandle(this->hSerial);
    }
}

int Serial::ReadData(char* buffer, unsigned int nbChar)
{
    //Number of bytes we'll have read
    DWORD bytesRead;
    //Number of bytes we'll really ask to read
    unsigned int toRead;

    //Use the ClearCommError function to get status info on the Serial port
    ClearCommErrors();

    //Check if there is something to read
    if (this->status.cbInQue > 0)
    {
        //If there is we check if there is enough data to read the required number
        //of characters, if not we'll read only the available characters to prevent
        //locking of the application.
        if (this->status.cbInQue > nbChar)
        {
            toRead = nbChar;
        }
        else
        {
            toRead = this->status.cbInQue;
        }

        //Try to read the require number of chars, and return the number of read bytes on success
        if (ReadFile(this->hSerial, buffer, toRead, &bytesRead, NULL))
        {
            return bytesRead;
        }

    }

    //If nothing has been read, or that an error was detected return 0
    return 0;

}


bool Serial::WriteData(const char* buffer, unsigned int nbChar)
{
    DWORD bytesSend;

    //Try to write the buffer on the Serial port
    if (!WriteFile(this->hSerial, (void*)buffer, nbChar, &bytesSend, 0))
    {
        //In case it don't work get comm error and return false
        ClearCommErrors();

        return false;
    }
    else
        return true;
}

bool Serial::IsConnected()
{
    //Simply return the connection status
    return this->connected;
}

void Serial::ClearCommErrors()
{
    ClearCommError(this->hSerial, &this->errors, &this->status);
}

std::string Serial::findComPort(const std::string comPortFriendlyName)
{
    bool found_port = false;
    string locPortName = "";
    string buf;
    logInfo("Searching for Arduino COM port.");

    vector<pair<UINT, string>> ports_info;
    if (SerialEnumserParts::QueryUsingSetupAPI(ports_info)) {
        logInfo("Found COM ports:");
        for (pair<int, string> e : ports_info) {
            buf = string("\t") + default_com_port_string + to_string(e.first) + ": " + e.second + ".";
            logInfo(buf.c_str());
            if (!found_port && e.second.find(comPortFriendlyName) != std::string::npos) {
                locPortName = default_com_port_string + to_string(e.first);
                logInfo(("\t\tFound Arduino port! " + locPortName).c_str());
                found_port = true;
            }
        }
    }
    else {
        logError("Problems iterating over ComPorts. Serial not set");
        return locPortName;
    }

    if (!found_port) {
        buf = "Could not find COM port with friendly name " + comPortFriendlyName + ". Aborting LED setup.";
        logError(buf.c_str());
    }

    return locPortName;
}
