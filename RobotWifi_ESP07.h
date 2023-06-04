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

#ifndef _ROBOTWIFI_ESP07_H_
#define _ROBOTWIFI_ESP07_H_
#include "Arduino.h"
// includes file not in repository with ssid and pwd
#include "secDefine1.h"

#include <SPI.h>

#include <RadioCommon.h>
#include <RH_RF95.h>

#include <RobotSharedDefines.h>

#include "ESP8266WiFi.h"
#include <ArduinoOTA.h>
#include <StreamParser.h>
// includes file not in repository with githash of current commit
#include "githash.h"



/*
 *
 *       ESP-07
 *
 *                                   +---------\/--------+
 *                                   |             GPIO01|22  TX
 *                                  2| A0          GPIO03|21  RX
 *                                   |             GPIO05|20  RF95 - EN
 *                  MainBrain Reset 4| GPIO16      GPIO04|19  RF95 - G0
 *                             SCK  5| GPIO14     GPIO00 |18  RF95 - RST
 *                            MISO  6| GPIO12     GPIO02 |17  ONBOARD_LED
 *                            MOSI  7| GPIO13     GPIO15 |16  RF95 - CS
 *                            VCC -- |                   | -- GND
 *                                   +-------------------+
 *
 *
 *
 *
 *
 */

#define RFM95_EN 5
#define RFM95_CS 15
#define RFM95_RST 0
#define RFM95_INT 4

void startWifi();
void stopWifi();
void startRadio();
void stopRadio();

void setupWifiConnection();
void setup();
void loop();
void heartbeat();

void scanNetworks();

void connectToHome();
void connectToHomeExt();
void connectToBase();
void beTheAP();

void resetConnection();

void clearBlackout();

void handleClient(char*);
void handleClientRaw(char*);
void handleSerial(char*);
void handleSerialRaw(char*);
void waitOnRMB(char*);

void handleRadioCommand(char*);
void handleRawRadio(uint8_t*);

void sendToBase(char*);


#endif /* _ROBOTWIFI_ESP07_H_ */
