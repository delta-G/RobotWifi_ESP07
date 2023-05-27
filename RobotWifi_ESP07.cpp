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


enum States {BOOTUP, WAITING_ON_RMB, WAITING_ON_BASE_WIFI, WAITING_ON_BASE_RADIO, RUNNING_WIFI, RUNNING_RADIO} bootState;

RH_RF95 radio(RFM95_CS, RFM95_INT);

const uint8_t radioSelectDIP = 16;

const uint8_t heartbeatPin = 2;
uint16_t heartbeatDelay = 2000;
unsigned long lastMil = millis();
boolean lastConnected = false;

uint32_t lastCommandTime;
uint32_t commandTimeout = 20000;
boolean blackoutReported = false;

boolean radioMode = false;  // false is WiFi - true is LoRa Radio

boolean rmbActive = false;
boolean WiFiConnected = false;
boolean connectedToBase = false;

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


	// RFM95_EN pin LOW to kill power to it
	pinMode(RFM95_EN, OUTPUT);
	digitalWrite(RFM95_EN, LOW);

	// ESP Radio off for low power if Radio gets turned on
	WiFi.mode(WIFI_OFF);
	WiFi.forceSleepBegin();
	delay(1);

	initRadio(RFM95_RST);  // Just sets up the reset pin and variables

	pinMode(radioSelectDIP, INPUT);

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

	bootState = BOOTUP;

	Serial.begin(ROBOT_COM_BAUD);
	delay(500);

	radioMode = (digitalRead(radioSelectDIP) == HIGH)?false:true;
	DEBUG("Beginning");
//	if(radioMode){
//		stopWifi();
//		delay(10);
//		startRadio();
//	} else {
//		stopRadio();
//		delay(10);
//		startWifi();
//	}
//
//	DEBUG("Back from scan and setup");

	heartbeatDelay = 500;

	DEBUG("EndOfSetup");

	bootState = WAITING_ON_RMB;

	serialParser.setRawCallback(handleSerialRaw);
	clientParser.setRawCallback(handleClientRaw);

}

void loop() {

	heartbeat();
	serialParser.run();

	switch (bootState) {

	case WAITING_ON_RMB: {
		if(rmbActive){   // gets set by Serial parser
			serialParser.setCallback(handleSerial);
			Serial.print(COM_START_STRING);
			if(radioMode){
				startRadio();
				bootState = WAITING_ON_BASE_RADIO;
			} else {
				startWifi();
				bootState = WAITING_ON_BASE_WIFI;
			}
			DEBUG(".");
		}
		break;
	}
	case WAITING_ON_BASE_WIFI:
		if(!client.connected()) {
			client = server.available();
		}
		if (connectedToBase) {
			Serial.print(F(COM_CONNECT_STRING));
			heartbeatDelay = 2000;
			bootState = RUNNING_WIFI;
			lastConnected = true;
		}
		clientParser.run();
		break;
	case WAITING_ON_BASE_RADIO:
		if (connectedToBase) {
			Serial.print(F(COM_CONNECT_STRING));
			heartbeatDelay = 2000;
			lastCommandTime = millis();
			bootState = RUNNING_RADIO;
		}
		listenToRadio();
		handleOutput();
		break;
	case RUNNING_WIFI: {
		if (!client.connected()) {
			if (lastConnected == true) {
				Serial.print("<LOST_COM>");
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
				lastConnected = true;
			}
			if (millis() - lastCommandTime >= commandTimeout) {
				if(!blackoutReported) {
					Serial.print(F("<LOST_COM>"));
					blackoutReported = true;
					heartbeatDelay = 200;
				}
			}
		}
		clientParser.run();
		break;
	}
	case RUNNING_RADIO: {

		listenToRadio();
		handleOutput();
		// if we lose contact, report to main brain and flashy light
		if (millis() - lastCommandTime >= commandTimeout) {
			if (!blackoutReported) {
				Serial.print(F("<LOST_COM>"));
				blackoutReported = true;
				heartbeatDelay = 200;
			}
		}

		break;
	}
	default: {
		//we shouldn't be here - Freak out
		heartbeatDelay = 50;
		break;
	}

	}

}

void clearBlackout(){
	blackoutReported = false;
	heartbeatDelay = 2000;
}

void heartbeat() {
	static boolean heartState = false;

	//static unsigned long lastMil = millis();
	unsigned long curMil = millis();

	if (curMil - lastMil >= heartbeatDelay) {
		heartState = !heartState;
		digitalWrite(heartbeatPin, heartState);
		lastMil = curMil;
	}
}

void startWifi(){
	if ((bootState == RUNNING_RADIO) || (bootState == WAITING_ON_BASE_RADIO)) {
		// kill radio first
		stopRadio();
		if (bootState == WAITING_ON_BASE_RADIO) {
			bootState = WAITING_ON_BASE_WIFI;
		} else {
			bootState = RUNNING_WIFI;
		}
	}

	WiFi.forceSleepWake();
	delay(1);

	setupWifiConnection();
	WiFiConnected = true;

	server.begin();
}

void stopWifi() {
	WiFi.disconnect(true);
	delay(1);
	WiFi.mode(WIFI_OFF);
	WiFi.forceSleepBegin();
	delay(1);
	WiFiConnected = false;
}

void startRadio(){
	if((bootState == RUNNING_WIFI) || (bootState == WAITING_ON_BASE_WIFI)){
		// kill wifi first
		stopWifi();
		if(bootState == WAITING_ON_BASE_WIFI){
			bootState = WAITING_ON_BASE_RADIO;
		} else {
			bootState = RUNNING_RADIO;
		}
	}
	// code to start the radio
	digitalWrite(RFM95_EN, HIGH); // enable pin HIGH to power radio
	delay(1);
	resetRadio();
	delay(1);
}

void stopRadio() {
	digitalWrite(RFM95_EN, LOW);
	delay(1);
}

void scanNetworks(){

	int count = WiFi.scanNetworks();

	for (int i = 0; i < count; i++){
		char buf[100] = {0};
		int len = WiFi.SSID(i).length() + 1;
		char sbuf[len];
		WiFi.SSID(i).toCharArray(sbuf, len);

		sprintf(buf, "<%s,%d>",sbuf , WiFi.RSSI(i));
//		client.print(buf);
		sendToBase(buf);
	}

}

void sendToBase(char* aBuf){
	if((bootState == RUNNING_RADIO) || (bootState == WAITING_ON_BASE_RADIO)){
		addToHolding(aBuf);
	} else if((bootState == RUNNING_WIFI) || (bootState == WAITING_ON_BASE_WIFI)){
		client.print(aBuf);
	}
}

void handleRadioCommand(char* aBuf){
	handleClient(aBuf);
}

void handleClient(char* aBuf){
	//   'E' denotes commands for the ESP8266
	if(!strcmp(aBuf, "<LGO>")){
		rmbActive = true;
	} else if(aBuf[1] == 'l'){
		if((bootState == RUNNING_RADIO) || (bootState == WAITING_ON_BASE_RADIO)){
			delay(250);
			handleConfigString(aBuf);
			delay(500);
		}
	} else if (aBuf[1] == 'P') {
		if ((bootState == RUNNING_RADIO) || (bootState == WAITING_ON_BASE_RADIO)) {
			int rvl = atoi((const char*) (aBuf + 2));
			char resp[10];
			snprintf(resp, 10, "<p%i>", rvl);
			sendToRadio(resp);
		}
	}
	else if(aBuf[1] == 'E'){
		switch(aBuf[2]){
		case 'P':
		{
			int rvl = atoi((const char*) (aBuf + 3));
			char resp[10];
			snprintf(resp, 10, "<p%i>", rvl);
//			client.print(resp);
			sendToBase(resp);
			break;
		}
		case 'G':
		{
			char gitbuf[9] = {0};
			strncpy(gitbuf, GIT_HASH, 8);
			gitbuf[8] = 0;
			char nbuf[20] = {0};
			sprintf(nbuf, "<EGit-%s>", gitbuf);

//			String notif = String("<E GitHash - ") + GIT_HASH + ">";
//			client.print(notif);
			sendToBase(nbuf);
			break;
		}
		case 'W':
			scanNetworks();
			break;
		case 'C': {
			if ((bootState == RUNNING_WIFI)	|| (bootState == WAITING_ON_BASE_WIFI)) {
				String notif = "<E  NewClient @ " + WiFi.SSID() + "," + WiFi.RSSI() + ">";
				client.print(notif);
			} else {
				sendToBase("<NotOnWifi>");
			}
			break;
		}
		case 'R':
			if(aBuf[3]=='r'){
//				stopWifi();  // startRadio does this right now but shouldn't.
				startRadio();
			}
			else if(aBuf[3]=='w'){
//				stopRadio(); // startWiFi does this right now but shouldn't.
				flush();
				delay(10);
				startWifi();
			}
			break;
		case 'X':
			// Disconnect, scan and reconnect
			resetConnection();
			break;

		default:
//			client.print("<Bad ESP>");
			sendToBase("<Bad ESP>");
		}
	}
	else if(rmbActive){
		//  Everything else goes to Main Brain
		Serial.print(aBuf);
	}
	connectedToBase = true;
	lastCommandTime = millis();
	if(blackoutReported){
		clearBlackout();
	}
}

void handleClientRaw(char* aBuf){
	uint8_t numBytes = aBuf[2];
	// if properly formatted message
	if((aBuf[0] == '<') && (aBuf[numBytes - 1] == '>')) {
		connectedToBase = true;  //  got a good message must be connected
		if (rmbActive) {
			for (uint8_t i = 0; i < numBytes; i++) {
				Serial.write(aBuf[i]);
			}
		}
	}
	lastCommandTime = millis();
	if(blackoutReported){
		clearBlackout();
	}
}

void handleRawRadio(uint8_t *p) {
	handleClientRaw((char*)p);
}

void handleSerial(char* aBuf) {
	if (!strcmp(aBuf, "<FFF>")){
		DEBUG("Flush Command");
		if((bootState == RUNNING_RADIO) || (bootState == WAITING_ON_BASE_RADIO)){
			flush();
			return;
		}
	} else if (aBuf[1] == 'l') {
		if ((bootState == RUNNING_RADIO) || (bootState == WAITING_ON_BASE_RADIO)) {
			addToHolding(aBuf);
			flush();
			handleConfigString(aBuf);
		}
	}
	else if (aBuf[1] == 'E') {
		switch (aBuf[2]) {
		case 'H':
			Serial.print("<^_^>");
			break;
		case 'd':
			connectedToBase = true;  // Backdoor when hooked to computer
			break;
		default:
			break;
		}
	} else {
		if ((bootState == RUNNING_WIFI) || (bootState == WAITING_ON_BASE_WIFI)) {
			client.print(aBuf);
		} else if ((bootState == RUNNING_RADIO) || (bootState == WAITING_ON_BASE_RADIO)) {
			if (connectedToBase) {
				addToHolding ((char*)aBuf);
			}
		}
	}
}


void handleSerialRaw(char *aBuf) {
	uint8_t numBytes = aBuf[2];

	if ((bootState == RUNNING_WIFI) || (bootState == WAITING_ON_BASE_WIFI)) {

		if (aBuf[1] == 0x13 && numBytes == ROBOT_DATA_DUMP_SIZE) {
			int32_t rssi = WiFi.RSSI();
			for (int i = 2; i <= 5; i++) {
				aBuf[ROBOT_DATA_DUMP_SIZE - i] = rssi & 0xFF;
				rssi >>= 8;
			}
		}
		for (int i = 0; i < numBytes; i++) {
			client.write(aBuf[i]);
		}
	} else if ((bootState == RUNNING_RADIO) || (bootState == WAITING_ON_BASE_RADIO)) {

		if (aBuf[1] == 0x13 && numBytes == ROBOT_DATA_DUMP_SIZE) {
			uint8_t snr = (uint8_t) (radio.lastSNR());
			int rs = radio.lastRssi();
			uint8_t rssi = (uint8_t) (abs(rs));
			aBuf[ROBOT_DATA_DUMP_SIZE - 5] = snr;
			aBuf[ROBOT_DATA_DUMP_SIZE - 4] = rssi;
		}

		addToHolding((uint8_t*) aBuf, numBytes);

		flush();

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

void setupWifiConnection() {
	DEBUG("scanAndSetup");
	connectToHome();
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

void resetConnection() {
	if ((bootState == RUNNING_WIFI) || (bootState == WAITING_ON_BASE_WIFI)) {
		client.stop();
		WiFi.disconnect();
		delay(5000);
		startWifi();
	}
	else if ((bootState == RUNNING_RADIO) || (bootState == WAITING_ON_BASE_RADIO)) {
		resetRadio();
	}
}

