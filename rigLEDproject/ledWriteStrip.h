#ifndef LEDWRITESTRIP_H_INCLUDED
#define LEDWRITESTRIP_H_INCLUDED

#include "SerialClass.h"

#define INPUT_SIZE 19
#define LED_OFF 0
#define LED_LEFT 1
#define LED_RIGHT 2
#define LED_BOTH 3
#define MAX_LED 40

const char *LED_PORT = "\\\\.\\COM4";


/*
Simple function to write to the LED strip for an Arduino, uses the SerialClass library for windows,
found at the arduino playground, https://playground.arduino.cc/Interfacing/CPPWindows/
Only relies on stdlib and windows.h which all windows computers should use.

side: Which LED strip to write to, either LED_OFF, LED_LEFT, LED_RIGHT, or LED_BOTH
num_lights: 0 <= int <= 40, number of LEDS to light up on the strip
red: 0 < int < 256, value for R
green: 0 < int < 256 value for G
blue: 0 < int < 256 value for B
*/

bool sendLedData(int side, int bottom_light = 0, int top_light = MAX_LED, int red = 0, int green = 0, int blue = 0)
{
    static Serial* SP = new Serial(LED_PORT); //Gets initialized first call and is static so only gets alloced once

    if (top_light > MAX_LED) //Quick error handling, lets not return errors unless write fails so just correct errors
        top_light = MAX_LED;
    if(bottom_light < 0)
        bottom_light = 0;

    red %= 256;
    green %= 256;
    blue %= 256;

    if (!SP->IsConnected()) //Make sure we are connected to the COM port, otherwise Write fails badly
        return false;

    char buff[INPUT_SIZE + 1]; //String saftey, not necessary but doesn't detract from anything by having it
    buff[INPUT_SIZE] = '\0';

    //Time for some manual buffer writing, not too bad because it is
    //a strict predetermined format but you know it is what it is
    //Uses super basic ascii math, if you don't understand this gitgud

    buff[0] = 48 + side; //Ascii 0 is 48
    buff[1] = ':';
    buff[2] = 48 + (bottom_light / 10);
    buff[3] = 48 + (bottom_light % 10);
    buff[4] = ':';
    buff[5] = 48 + (top_light / 10);
    buff[6] = 48 + (top_light % 10);
    buff[7] = ':';
    buff[8] = 48 + (red / 100);
    buff[9] = 48 + ((red % 100) / 10);
    buff[10] = 48 + (red % 10);
    buff[11] = ':';
    buff[12] = 48 + (green / 100);
    buff[13] = 48 + ((green % 100) / 10);
    buff[14] = 48 + (green % 10);
    buff[15] = ':';
    buff[16] = 48 + (blue / 100);
    buff[17] = 48 + ((blue % 100) / 10);
    buff[18] = 48 + (blue % 10);

    bool ret = SP->WriteData(buff, INPUT_SIZE); //Actually do the writing to serial port
    return ret;
}

#endif