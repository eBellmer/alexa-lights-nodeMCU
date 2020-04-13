/*
  Title: 
  Description: 
  Author: Ethan Bellmer
  Date: 13/04/2020
  Version: 2.0
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "switch.h"
#include "UpnpBroadcastResponder.h"
#include "CallbackFunction.h"

// Set WiFi SSID & password before flashing
const char* ssid = "SSID";
const char* password = "PWD";

// Declare pin position for LED, relay, and push button
int ledPin = 2;
int relayPin = 5;
int buttonPin = 16;

// Instatiate with a false power and button sate.
// This is used for tracking power on/off state between button and Alexa uses. 
boolean powerState = false;
int lastButtonState = 0;

// wifi connection
boolean connectWifi();
boolean wifiConnected = false;

// Alexa on/off callbacks 
void lightsOn();
void lightsOff();

// wemo Switches
Switch *lights = NULL;

UpnpBroadcastResponder upnpBroadcastResponder;

void setup() 
{
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT);

  digitalWrite(ledPin, HIGH);
  digitalWrite(relayPin, LOW);
  // initialise wifi connection
  wifiConnected = connectWifi();
  
  if(wifiConnected) 
  {
    upnpBroadcastResponder.beginUdpMulticast();
    
    // Define your switches here. Max 14
    // Format: Alexa invocation name, local port no, on callback, off callback
    lights = new Switch("Lights", 8000, lightsOn, lightsOff);

    Serial.println("Adding switches to upnp broadcast responder...");
    upnpBroadcastResponder.addDevice(*lights);
  }
}
 
void loop() 
{
  if(wifiConnected) 
  {
    upnpBroadcastResponder.serverLoop();
    lights->serverLoop();
  }

  // check if button is a new press or a continuation from last loop
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

// connect to wifi – returns true if successful or false if not
boolean connectWifi()
{
  boolean state = true;
  int i = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");
  
  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  if (i > 10)
  {
    state = false;
    break;
  }
    i++;
  }
  
  if (state)
  {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } 
  else 
  {
    Serial.println("");
    Serial.println("Connection failed.");
  }
  return state;
}
