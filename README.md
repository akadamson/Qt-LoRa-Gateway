# Qt-LoRa-Gateway
Qt based serial LoRa Gateway

History:
* 2016-11-12: Added a Debug History window, SSDV display window, and Window menu item.  SSDV display will be scaled to 320x240 for now, I'll fix this in a later version.  Both Debug History and SSDV are live from program start and do not require the upload check box to be set.  History window is structured in sysylogd format
* 2016-10-26: Removed debugging code in initial version and rebuilt
* 2016-10-25: Initial commit - Windows prebuilt binary included in zipfile

Inspired and most logic cloned from https://github.com/daveake/LoRaSerialGateway

I attempted to use Dave's excellent serial gateway that was built in Delphi, but it wouldn't talk
to the VCP type serial port on an Adafruit Feather.  As a result, I moved most of the logic over to
Qt5, and with the help of another friend, glued in some serial support.

The results are a basic clone of Dave's version, but in Qt5, which should allow others to clone 
this and add either Mac, or Linux forms and build and run the source on either of those platforms

To utilize, you need an arduino like board with LoRa module and Daves other code - https://github.com/daveake/LoRaArduinoSerial
NOTE: For me this was easiest to do using an Adafruit Feather with LoRa:  To utilize change _slaveSelectPin = 8, and dio5 = 10.  Then jumper, D10 on the feather to the IO5 pin next to the LoRa module.  Program the Arduino with these changes, and use the QT gateway code to connect to the serial port that the arduino will present.

KNOWN ISSUES:
a) The SSDV upload is one frame at a time, I modified it for 3 frames at a time, but it seemed
too fragile and I didn't spend any more time with it
b) The serial support is way overkill, it could be tuned to reduce the memory footprint, but again
it works so I didn't mess with it.

