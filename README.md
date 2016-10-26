# Qt-LoRa-Gateway
Qt based serial LoRa Gateway

History:
2016-10-25: Initial commit

inspired and most logic cloned from https://github.com/daveake/LoRaSerialGateway

I attempted to use Dave's excellent serial gateway that was built in Delphi, but it wouldn't talk
to the VCP type serial port on an Adafruit Feather.  As a result, I move most of the logic over to
Qt5, and with the help of another friend, glued in some serial support.

The results are a basic clone of Dave's version, but in Qt5, which should allow others to clone 
this and add either Mac, or Linux forms and build and run the source on either of those platforms

KNOWN ISSUES:
a) The SSDV upload is a one frame at a time, I modified it for 3 frames at a time, but it seemed
too fragile and I didn't spend any more time with it
b) The serial support is way overkill, it could be tuned to reduce the memory footprint, but again
it works so I didn't mess with it.

