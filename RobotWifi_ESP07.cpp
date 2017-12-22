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

const uint8_t heartbeatPin = 12;
uint16_t heartbeatDelay = 2000;
unsigned long lastMil = millis();

const char* ssid = MY_NETWORK_SSID;
const char* pwd = MY_NETWORK_PASSWORD;

WiFiServer server(1234);
WiFiClient client;

StreamParser serialParser(&Serial, START_OF_PACKET, END_OF_PACKET, handleSerial);
StreamParser clientParser(&client, START_OF_PACKET, END_OF_PACKET, handleClient);



void setupWiFi() {

//	IPAddress ipa(192, 168, 1, 75);
////	IPAddress dns(192, 168, 1, 1);
//	IPAddress gate(192, 168, 1, 1);
//	IPAddress sub(255, 255, 255, 0);

//	Serial.println("Setting Up WiFi");

//	WiFi.mode(WIFI_STA);
//	WiFi.config(ipa, gate, sub);
//	WiFi.begin(ssid, pwd);
//
//	heartbeatDelay = 100;
//
//	while (WiFi.status() != WL_CONNECTED) {
//		delay(500);
//		Serial.print(".");
//		heartbeat();
//	}

	//  This section for AP Mode with static IP. (Untested!!!)


//	WiFi.mode(WIFI_AP_STA);
//	WiFi.softAPConfig(ipa, gate, sub);
//	WiFi.softAP("RControl");




	// This section is AP Mode with 192.168.4.1
	WiFi.mode(WIFI_AP);
	WiFi.softAP("RControl");
//	Serial.println("");
//	Serial.println(WiFi.localIP());

	delay(500);

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

	setupWiFi();

	server.begin();

	heartbeatDelay = 250;

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
		client = server.available();
		heartbeatDelay = 200;
	} else {
		heartbeatDelay = 2000;

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
				client.print("<ESP-HB ");
				client.print(WiFi.RSSI());
				client.print(">");
			}
	}
}


void scanNetworks(){

	int count = WiFi.scanNetworks();

	for (int i = 0; i < count; i++){
		char buf[100] = {0};
		sprintf(buf, "<%d,%d>", WiFi.SSID(i), WiFi.RSSI(i));
		client.print(buf);
	}

}


void handleClient(char* aBuf){

	if(aBuf[1] == 'E'){
		switch(aBuf[2]){
		case 'W':
			scanNetworks();
			break;
		default:
			client.print("<Bad Command>");
		}
	}
	else {
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



