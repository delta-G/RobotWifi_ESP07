/*

RobotWifi-ESP07  --  runs on ESP8266 and handles WiFi communications for my robot
     Copyright (C) 2017  David C.

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.

     */
/*
 *
 * Written for ESP-07 board.
 *
 * Serves as a simple serial converter.
 * Allows me to communicate to an Arduino that expects
 * a serial connection using a TCP socket.
 *
 *
 */


#include "RobotWifi_ESP07.h"


//#define DEBUG_OUT Serial

#ifdef DEBUG_OUT
#define DEBUG(x) DEBUG_OUT.println(x)
#else
#define DEBUG(x)
#endif


enum States {BOOTUP, WAITING_ON_RMB, RUNNING} bootState;

const uint8_t heartbeatPin = 12;
uint16_t heartbeatDelay = 2000;
unsigned long lastMil = millis();
boolean lastConnected = false;

boolean rmbActive = false;
boolean WiFiConnected = false;

const char* ssid = MY_NETWORK_SSID;
const char* pwd = MY_NETWORK_PASSWORD;

WiFiServer server(ROBOT_PORT);
WiFiClient client;

StreamParser serialParser(&Serial, START_OF_PACKET, END_OF_PACKET, waitOnRMB);
StreamParser clientParser(&client, START_OF_PACKET, END_OF_PACKET, handleClient);




/**********************************************************
 *
 * Right off the bat there is 1.5 seconds of delay as the LED bumps
 *
 * Serial is connected at 1.5 seconds
 *
 * Go to connect WiFi at 2.0 seconds
 *
 **********************************************************/

void setup() {

	pinMode(heartbeatPin, OUTPUT);
	digitalWrite(heartbeatPin, HIGH);
	delay(500);
	digitalWrite(heartbeatPin, LOW);
	delay(250);
	digitalWrite(heartbeatPin, HIGH);
	delay(500);
	digitalWrite(heartbeatPin, LOW);
	delay(250);
	digitalWrite(heartbeatPin, HIGH);


	Serial.begin(ROBOT_COM_BAUD);
	delay(500);

	DEBUG("Beginning");

	setupWifi();
	DEBUG("Back from scan and setup");

	WiFiConnected = true;


	server.begin();

	heartbeatDelay = 500;

	DEBUG("EndOfSetup");

	bootState = WAITING_ON_RMB;

}

void loop() {

	heartbeat();

	switch (bootState) {

	case WAITING_ON_RMB: {
		if(rmbActive){   // gets set by Serial parser
			serialParser.setCallback(handleSerial);
			Serial.print(COM_START_STRING);
			Serial.print(COM_CONNECT_STRING);
			bootState = RUNNING;
			DEBUG(".");
		}
		break;
	}
	case RUNNING: {
		if (!client.connected()) {
			if (lastConnected == true) {
				DEBUG("LOST CONNECTION");
				// If we just lost connection kill the motors.
				// TODO:  We should just tell main brain that
				// we lost connection and let him figure out
				// what to do
				Serial.print("<ML,0>");
				Serial.print("<MR,0>");
			}
			client = server.available();
			heartbeatDelay = 200;
			lastConnected = false;
		} else {
			if (lastConnected == false) {
				// if we just now regained connection
				String notif = "<E  NewClient @ " + WiFi.SSID() + ","
						+ WiFi.RSSI() + ">";
				DEBUG(notif);
				client.print(notif);
			}
			heartbeatDelay = 2000;
			lastConnected = true;
		}
		break;
	}
	default: {
		//we shouldn't be here - Freak out
		heartbeatDelay = 50;
		break;
	}

	}

	serialParser.run();
	clientParser.run();

}

void heartbeat() {
	static boolean heartState = false;

	//static unsigned long lastMil = millis();
	unsigned long curMil = millis();

	if (curMil - lastMil >= heartbeatDelay) {
		heartState = !heartState;
		digitalWrite(heartbeatPin, heartState);
		lastMil = curMil;
		if (client.connected()) {
				client.print("<E-HB");
				client.print(WiFi.RSSI());
				client.print(">");
			}
	}
}


void scanNetworks(){

	int count = WiFi.scanNetworks();

	for (int i = 0; i < count; i++){
		char buf[100] = {0};
		int len = WiFi.SSID(i).length() + 1;
		char sbuf[len];
		WiFi.SSID(i).toCharArray(sbuf, len);

		sprintf(buf, "<%s,%d>",sbuf , WiFi.RSSI(i));
		client.print(buf);
	}

}


void handleClient(char* aBuf){
	//   'E' denotes commands for the ESP8266
	if(aBuf[1] == 'E'){
		switch(aBuf[2]){
		case 'G':
		{
			String notif = String("<E GitHash - ") + GIT_HASH + ">";
			client.print(notif);
			break;
		}
		case 'W':
			scanNetworks();
			break;
		case 'C':
		{
			String notif = "<E  NewClient @ " + WiFi.SSID() + "," + WiFi.RSSI() + ">";
			client.print(notif);
			break;
		}
		case 'X':
			// Disconnect, scan and reconnect
			killConnection();
			break;

		default:
			client.print("Bad Command ESP");
		}
	}
	else {
		//  Everything else goes to Main Brain
		Serial.print(aBuf);
	}
}

void handleSerial(char* aBuf) {
	if (aBuf[1] == 'E') {
		switch (aBuf[2]) {
		case 'H':
			Serial.print("<^_^>");
			break;
		default:
			break;
		}
	} else {
		client.print(aBuf);
	}
}

void waitOnRMB(char* aBuf) {
	if (strcmp(aBuf, RMB_STARTUP_STRING) == 0){
		rmbActive = true;
	}
}


////   **TODO:
/*
 * This code should build an array or linked list or something of all the SSID that it finds.
 * Or at least the ones it recognizes or aren't secure
 * Then we can parse through that list later and decide what we want to connect to
 *
 */

void setupWifi() {

	DEBUG("scanAndSetup");


	int count = WiFi.scanNetworks();

	DEBUG(count);

	static int homeStrength = -10000;
	static int extStrength = -10000;

	for (int i = 0; i < count; i++) {

		if(WiFi.SSID(i).lastIndexOf("Disco_Bot_Base") > -1){
			DEBUG("FOUND-BASE-WIFI");
			connectToBase();
			return;
		}

		else if(WiFi.SSID(i).lastIndexOf("Disco_Radio_EXT") > -1){
			DEBUG("FOUND-EXT-WIFI");
			extStrength = WiFi.RSSI(i);
		}

		else if(WiFi.SSID(i).lastIndexOf("Disco_Radio") > -1){
			DEBUG("FOUND-HOME-WIFI");
			homeStrength = WiFi.RSSI(i);
		}
	}

	//  If we got here then we didn't find Disco_Bot_Base
	//  so see about other networks.
	if (extStrength || homeStrength) {
		if (extStrength > homeStrength) {
			connectToHomeExt();
		} else {
			connectToHome();
		}
	} else {
		beTheAP();
	}

}

void beTheAP() {
	// This function is AP Mode with 192.168.4.1
	DEBUG("be the AP");
	WiFi.mode(WIFI_AP);
	WiFi.softAP("RControl");
}

void connectToBase() {
	IPAddress ipa(10, 10, 0, 24);
	IPAddress gate(10, 10, 0, 1);
	IPAddress sub(255, 255, 255, 0);

	WiFi.mode(WIFI_STA);
	WiFi.config(ipa, gate, sub);
	WiFi.begin("Disco_Bot_Base", "12341234");

	heartbeatDelay = 100;

	while (WiFi.status() != WL_CONNECTED) {
		delay(50);
		heartbeat();
	}
}

void connectToHome() {
	DEBUG("connecting to home");

	IPAddress ipa(ROBOT_IP);
	IPAddress gate(MY_DEFAULT_GATEWAY);
	IPAddress sub(MY_NETMASK);


	WiFi.mode(WIFI_STA);
	WiFi.config(ipa, gate, sub);
	WiFi.begin(ssid, pwd);

	heartbeatDelay = 100;

	while (WiFi.status() != WL_CONNECTED) {
		delay(50);
		heartbeat();
	}
}


void connectToHomeExt() {
	DEBUG("connecting to ext");

	IPAddress ipa(ROBOT_IP);
	IPAddress gate(MY_DEFAULT_GATEWAY);
	IPAddress sub(MY_NETMASK);


	WiFi.mode(WIFI_STA);
	WiFi.config(ipa, gate, sub);

	char extssid[50];

	strcpy(extssid, ssid);

	int i = strlen(ssid);

	strcpy (extssid + i, "_EXT");

	WiFi.begin(extssid, pwd);

	heartbeatDelay = 100;

	while (WiFi.status() != WL_CONNECTED) {
		delay(50);
		heartbeat();
	}
}

void killConnection() {
	client.stop();
	WiFi.disconnect();
	delay(5000);
	setupWifi();
	server.begin();
}

