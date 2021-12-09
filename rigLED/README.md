INSTALATION INSTRUCTIONS:
Requires 3 things: Windows Computer, Arduino, FastLED library.
Arduino: https://www.arduino.cc/en/Guide/Windows
FastLED: https://github.com/FastLED/FastLED/releases

Unzip the FastLED library into the Arduino Libraries folder, usually at C:/Program Files (x86)/Arduino/libraries
Open the rigLED.ino folder, got to Sketch->Include Library->Contributed Library and make sure FastLED is recognized, if it is then words like FastLED and CRGB should be orange instead of the default text color.
Upload code to the arduino Uno Board.
It is easy to test if the code is working, by using the serial monitor to send messages such as 
1:40:255:000:000 or 2:40:000:255:000 or 3:40:000:000:255, make sure to close the serial monitor after testing. 
Check what COM the arduino is on, Set that in ledWriteStrip.h, replacing COM4 with whatever COM port it is
use #include "ledWriteStrip.h" to gain access to sendLedData(side, bottom_light, top_light, red, green, blue) which you can use anywhere in CPP code.
There is no allocation needed anywhere, just use that function call, it uses the same COM port as the board so it will not work if the board is not plugged in or the SerialMonitor is open.


The function is used as such, sendLedData(side, bottom_light, top_light, red, green, blue), will light a strip of LEDs on side, within the range bottom-top which goes 0 <= MAX_LEDS.
red, green and blue are ints within range 0-255, you can get any color at any brightness by mixing and matching these values (128,0,0) is dimmer red then (255,0,0), I don't know if
the brightness is linear I didn't have a photo measuring device.
bottom defualts to 0, top defaults to MAX_LED, and red, green and blue default to 0, which means it is very easy to turn the LEDS off just sendLedData(LED_LEFT); will turn off the left strip,
while sendLedData(LED_OFF); will turn off both. Using LED_BOTH will write to same color to both strips, if you want different colors just send two commands in succession.

See ledWriteStrip for more in depth comments.
