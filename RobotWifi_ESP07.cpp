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


#define USE_HOME_WIFI
//#define USE_ESP_AS_HOTSPOT_STATIC
//#define USE_ESP_AS_HOTSPOT
//#define USE_BASE_STATION_WIFI


const uint8_t heartbeatPin = 12;
uint16_t heartbeatDelay = 2000;
unsigned long lastMil = millis();
boolean lastConnected = false;

const char* ssid = MY_NETWORK_SSID;
const char* pwd = MY_NETWORK_PASSWORD;

WiFiServer server(1234);
WiFiClient client;

StreamParser serialParser(&Serial, START_OF_PACKET, END_OF_PACKET, handleSerial);
StreamParser clientParser(&client, START_OF_PACKET, END_OF_PACKET, handleClient);



void setupWiFi() {

#ifdef USE_HOME_WIFI

	IPAddress ipa(192, 168, 1, 75);
	IPAddress gate(192, 168, 1, 1);
	IPAddress sub(255, 255, 255, 0);

//	Serial.println("Setting Up WiFi");

	WiFi.mode(WIFI_STA);
	WiFi.config(ipa, gate, sub);
	WiFi.begin(ssid, pwd);

	heartbeatDelay = 100;

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		heartbeat();
	}

#endif

#ifdef USE_BASE_STATION_WIFI

	IPAddress ipa(10, 10, 0, 24);
	IPAddress gate(10, 10, 0, 1);
	IPAddress sub(255, 255, 255, 0);

	WiFi.mode(WIFI_STA);
		WiFi.config(ipa, gate, sub);
		WiFi.begin("Disco_Bot_Base", "12341234");

		heartbeatDelay = 100;

		while (WiFi.status() != WL_CONNECTED) {
			delay(500);
			Serial.print(".");
			heartbeat();
		}

#endif


#ifdef USE_ESP_AS_HOTSPOT_STATIC
	//  This section for AP Mode with static IP. (Untested!!!)


//	WiFi.mode(WIFI_AP_STA);
//	WiFi.softAPConfig(ipa, gate, sub);
//	WiFi.softAP("RControl");

#endif

#ifdef USE_ESP_AS_HOTSPOT

	// This section is AP Mode with 192.168.4.1
	WiFi.mode(WIFI_AP);
	WiFi.softAP("RControl");
//	Serial.println("");
//	Serial.println(WiFi.localIP());
#endif





	delay(500);

}


void scanAndSetup() {

	int count = WiFi.scanNetworks();

	static int homeStrength;
	static int extStrength;

	for (int i = 0; i < count; i++) {

		if(WiFi.SSID(i).lastIndexOf("Disco_Bot_Base") > -1){
			connectToBase();
			return;
		}

		else if(WiFi.SSID(i).lastIndexOf("Disco_Radio_EXT") > -1){
			extStrength = WiFi.RSSI(i);
		}

		else if(WiFi.SSID(i).lastIndexOf("Disco_Radio") > -1){
			homeStrength = WiFi.RSSI(i);
		}
	}

	//  If we got here then we didn't find Disco_Bot_Base
	//  so see about other networks.
	if (extStrength > homeStrength ){
		connectToHomeExt();
	}
	else {
		connectToHome();
	}
}

void beTheAP() {
	// This function is AP Mode with 192.168.4.1
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
//		Serial.print(".");
		heartbeat();
	}
}

void connectToHome() {

	IPAddress ipa(192, 168, 1, 75);
	IPAddress gate(192, 168, 1, 1);
	IPAddress sub(255, 255, 255, 0);

	//	Serial.println("Setting Up WiFi");

	WiFi.mode(WIFI_STA);
	WiFi.config(ipa, gate, sub);
	WiFi.begin(ssid, pwd);

	heartbeatDelay = 100;

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		heartbeat();
	}
}


void connectToHomeExt() {

	IPAddress ipa(192, 168, 1, 75);
	IPAddress gate(192, 168, 1, 1);
	IPAddress sub(255, 255, 255, 0);

	//	Serial.println("Setting Up WiFi");

	WiFi.mode(WIFI_STA);
	WiFi.config(ipa, gate, sub);

	char extssid[50];

	strcpy(extssid, ssid);

	int i = strlen(ssid);

	strcpy (extssid + i, "_EXT");

	WiFi.begin(extssid, pwd);

	heartbeatDelay = 100;

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		heartbeat();
	}
}

void killConnection() {
	client.stop();
	WiFi.disconnect();
	delay(5000);
	scanAndSetup();
	server.begin();
}



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

	Serial.begin(115200);
	delay(500);

//	setupWiFi();
	scanAndSetup();

	server.begin();

//	while (!client.connected()) {
//		client = server.available();
//		heartbeat();
//	}

	heartbeatDelay = 2000;
	//client.print("<Connected ESP07>");

}

void loop() {

	heartbeat();

	if (!client.connected()) {
		if(lastConnected == true){
			// If we just lost connection kill the motors.
			Serial.print("<ML,0>");
			Serial.print("<MR,0>");
		}
		client = server.available();
		heartbeatDelay = 200;
		lastConnected = false;
	} else {
		if(lastConnected == false){
			// if we just now regained connection
			String notif = "<E  NewClient @ " + WiFi.SSID() + "," + WiFi.RSSI() + ">";
			client.print(notif);
		}
		heartbeatDelay = 2000;
		lastConnected = true;

		serialParser.run();
		clientParser.run();
	}
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
			client.print("<Bad Command>");
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



