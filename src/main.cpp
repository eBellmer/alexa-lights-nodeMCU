#include <Arduino.h>
#include <fauxmoESP.h>
#include <ESP8266WiFi.h>

// Include file that contains the SSID and password for the WiFi network
#include "credentials.h"


// Instatiate with a false power and button sate.
// This is used for tracking power on/off state between button and Alexa uses. 
boolean powerState = false;
int lastButtonState = 0;

// Declare pin position for LED, relay, and push button
#define SERIAL_BAUDRATE 115200
#define relayPin 5 // GPIO 5 = NodeMCU D1
#define relayID "office light"
#define ledPin 2 // GPIO 2 = NodeMCU D4
#define ledID "button LED"
#define buttonPin 16 // GPIO 16 = NodeMCU D0
#define buttonID "button pin"

// Instantiate new fauxmoESP object
fauxmoESP fauxmo;


/***
 * Turn on callback that turns on the relay for the lights
 * and turns off the indicator LED.
 */
void lightsOn() 
{
	digitalWrite(relayPin, HIGH); // turn on relay
	digitalWrite(ledPin, LOW); // turn off LED
	Serial.println("Turning on lights");
	powerState = true; // set the power state to on for the toggle
}

/***
 * Turn off callback that turns off the relay for the lights
 * and turns on the indicator LED.
 */
void lightsOff() 
{
	digitalWrite(relayPin, LOW); // turn off relay
	digitalWrite(ledPin, HIGH); // turn on LED
	Serial.println("Turning off lights");
	powerState = false; // set the power state to off for the toggle
}

void wifiSetup() {

	// Set WIFI module to STA mode
	WiFi.mode(WIFI_STA);

	// Connect
	Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
	WiFi.begin(WIFI_SSID, WIFI_PASS);

	// Wait
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		delay(100);
	}
	Serial.println();

	// Connected!
	Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

}

void setup() {
	Serial.begin(SERIAL_BAUDRATE);

	pinMode(ledPin, OUTPUT);
	pinMode(relayPin, OUTPUT);
	pinMode(buttonPin, INPUT);

	// Write ledPin high at boot so the LED is on when the lights are off.
	digitalWrite(ledPin, HIGH);
	// Write relay pin low at boot so lights remain off when module boots.
	digitalWrite(relayPin, LOW);

	// Run the WiFi setup function and connect to the specified WiFi network. 
    wifiSetup();


    // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);

    // You can use different ways to invoke alexa to modify the devices state:
    // "Alexa, turn yellow lamp on"
    // "Alexa, turn on yellow lamp
    // "Alexa, set yellow lamp to fifty" (50 means 50% of brightness, note, this example does not use this functionality)
	// Add virtual devices
    fauxmo.addDevice(relayID);


	fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        
        // Callback when a command from Alexa is received. 
        // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
        // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
        // Just remember not to delay too much here, this is a callback, exit as soon as possible.
        // If you have to do something more involved here set a flag and process it in your main loop.
        
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
        // Otherwise comparing the device_name is safer.

        digitalWrite(relayPin, state ? HIGH : LOW);
		digitalWrite(ledPin, state ? LOW : HIGH);

    });


}

void loop() {
	// fauxmoESP uses an async TCP server but a sync UDP server
    // Therefore, we have to manually poll for UDP packets
	fauxmo.handle();


	// This is a sample code to output free heap every 5 seconds
    // This is a cheap way to detect memory leaks
    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }

    // If your device state is changed by any other means (MQTT, physical button,...)
    // you can instruct the library to report the new state to Alexa on next request:
    // fauxmo.setState(ID_YELLOW, true, 255);

	// check if button is a new press or a continuation from last loop.
	if ((digitalRead(buttonPin) == LOW) && (lastButtonState == 1)) 
	{
		lastButtonState = 0;
		if(!powerState)
		{
			lightsOn(); // call lightsOn() if lights are off
		}
		else
		{
			lightsOff(); // call lightsOff() if lights are on
		}
	}
	else if ((digitalRead(buttonPin) == HIGH) && (lastButtonState == 0)) 
	{
		lastButtonState = 1; // button released
	}
  delay(100); // button bounce delay
}
