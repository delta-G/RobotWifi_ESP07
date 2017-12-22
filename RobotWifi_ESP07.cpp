#include "RobotWifi_ESP07.h"

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

const uint8_t heartbeatPin = 12;
uint16_t heartbeatDelay = 2000;
unsigned long lastMil = millis();

const char* ssid = MY_NETWORK_SSID;
const char* pwd = MY_NETWORK_PASSWORD;

WiFiServer server(1234);
WiFiClient client;

char replyBuffer[50];


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
		if (client.available() > 0) {

			Serial.print((char) client.read());

		}
		if (Serial.available()) {
			static uint8_t index;
			char c = Serial.read();
			replyBuffer[index] = c;
			replyBuffer[++index] = 0;

			if (c == '>' || index == 40){
				client.print(replyBuffer);
				index = 0;
				replyBuffer[index] = 0;

			}
//			client.print((char) Serial.read());
		}
		heartbeatDelay = 2000;
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
