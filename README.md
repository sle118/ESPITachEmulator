#iTach Emulator for the ESP8266

This library partially implements Global Cache's irsend protocol, including compression and repeats, on the ESP8266 platform. This allows infrared send/receive actions to be efficiently executed across a TCP/IP connection. The complete protocol is documented in the following PDF: https://www.globalcache.com/files/docs/API-iTach.pdf

Global Cache has an impressive line of products which are well worth their costs, and purchasing their product from one of their distributors.
https://www.globalcache.com/partners/distributors/north-america/

A special mention to the inspiration behind this, probonopd's emulator
https://github.com/probonopd/ESP8266iTachEmulator


#Building
The development was done usign the IDE found at http://platformio.org which makes resolving the dependencies a breeze.

The denendencies are (including the ones needed to build the example) 
* ArduinoJson
* ESPAsyncTCP
* ESPAsyncWebServer
* IRremoteESP8266
* RCSwitch
* ESPmanager
* https://github.com/sle118/IrServiceBase.git
* ProntoHex

Defining DEBUG_LEVEL_VERBOSE activates sending detailed debugging messages on the serial port as well as to any client connected to the debug port with telnet. It is strongly suggested to use serial debugging rather than Telnet when activating this option, as sending massive amounts of data to a tcpip client isn't optimized and could result in a wdt reset. 

#Example
The example implements a full fledged client with a captive portal that enables setting up the ESP8266's connection on the initial boot and managing the options afterwards. 
 
