

#include "ledWriteStrip.h"


int main()
{
    printf("Welcome to the serial test app!\n\n");

    for(int i = 0; i < 41; i++)
    {
        sendLedData(LED_LEFT, i, 255, 0, 0);
        Sleep(100);
    }
    Sleep(500);
    for(int i = 0; i < 41; i++)
    {
        sendLedData(LED_LEFT, i, 0, 255, 0);
        Sleep(100);
    }
    Sleep(500);
    for(int i = 0; i < 41; i++)
    {
        sendLedData(LED_RIGHT, i, 255, 0, 0);
        Sleep(100);
    }
    Sleep(500);
    for(int i = 0; i < 41; i++)
    {
        sendLedData(LED_RIGHT, i, 0, 255, 0);
        Sleep(100);
    }
    Sleep(500);
    sendLedData(LED_OFF, 0, 0, 0, 0);
    return 0;
}