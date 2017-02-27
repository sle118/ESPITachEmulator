/*
 Implements a subset of the iTach API Specification Version 1.5
 http://www.globalcache.com/files/docs/API-iTach.pdf
 and
 
 Tested with
 - IrScrutinizer 1.1.2 from https://github.com/bengtmartensson/harctoolboxbundle
 
 
 Loosely adapted from probonopd's repository
 https://github.com/probonopd/ESP8266iTachEmulator
 
 Circuit:
 To get good range, attach a resistor to pin 12 of the ESP-12E and connect the resistor to the G pin of a 2N7000 transistor.
 If you look at the flat side of the 2N7000 you have S, G, D pins.
 Connect S to GND, G to the resistor to the MCU, and D to the IR LED short pin.
 The long pin of the IR LED is connected to +3.3V.
 I picked the 2N7000 because unlike others it will not pull pin 2 down which would prevent the chip from booting.
 
 There are several circuit examples on the internet and a simple search for "Arduino infrared" or ESP8266" Infrared"
 will provide numerous results.  
 */

#include <Arduino.h>
#include <FS.h> //  Settings saved to SPIFFS
#include <RCSwitch.h>
#include <ESPmanager.h>
#include "Service.h"
// ESP SDK
extern "C" {
#include "user_interface.h"
}


#ifndef UNIT_TEST

#define INFRARED_LED_PIN 12 // ############# CHECK IF THE LED OR TRANSISTOR (RECOMMENDED) IS ACTUALLY ATTACHED TO THIS PIN
#define RECV_PIN  D1 // an IR detector/demodulatord
#define UDP_PORT 9131 // UPNP broadcasts are sent to this port
#define HTTP_SERVER_PORT 80 // Used by the portal to initialize the ESP's wifi settings
#define ITACH_SERVER_PORT 4998  // The port that the iTach emulator will listen to
#define DEBUG_LISTENER_PORT 22 // the debug port to send trace messages to 
using namespace iTach;
AsyncWebServer HTTP(HTTP_SERVER_PORT);
ESPmanager settings(HTTP, SPIFFS);
Service itach(UDP_PORT, ITACH_SERVER_PORT, INFRARED_LED_PIN, RECV_PIN,
		DEBUG_LISTENER_PORT, "Welcome debug client!");

time_t next_dump = millis() + 5000;


void setup() {

	Serial.begin(57600);
	Serial.printf("Sketch size: %u\n", ESP.getSketchSize());
	Serial.printf("Free size: %u\n", ESP.getFreeSketchSpace());
	time_t upTo = millis() + 3000;
	while (millis() < upTo && WiFi.status() != WL_CONNECTED)
		delay(50);
	settings.begin();

	//  This rewrite is active when the captive portal is working, and redirects the root / to the setup wizard.
	//  This has to go in the main sketch to allow your project to control the root when using ESPManager.
	HTTP.rewrite("/", "/espman/setup.htm").setFilter(
			[](AsyncWebServerRequest * request) {
				return settings.portal();
			});

	//  then use this rewrite and serve static to serve your index file(s)
	HTTP.rewrite("/", "/espman/"); // redirect other requests to espman's page for now
	
	HTTP.begin();

	itach.begin();

	if (settings.portal()) {
		softap_config config;
		wifi_softap_get_config(&config);
		IrServiceBase::debugSend(
				"Portal active. Connect to wifi: "
						+ String((char*) config.ssid)
						+ " and navigate to http://"
						+ WiFi.softAPIP().toString() + "/ \n");
	} else {
		IrServiceBase::debugSend(
				"Ready! Use IrScrutinizer to connect to "
						+ WiFi.localIP().toString() + ":"
						+ String(itach.Port()));
	}

	next_dump = millis() + 5000;

}

void loop() {

	settings.handle();

	IrServiceBase::processDecodeIrReceive();
	if (!settings.portal()) {
		itach.handleNewConnections();
		// the server should not hold process for more than a certain amount of time
		// or else wdt will be thrown
		itach.process(300);
	}

}

#endif
