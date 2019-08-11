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

#ifndef _RobotWifi_ESP07_H_
#define _RobotWifi_ESP07_H_
#include "Arduino.h"
// includes file not in repository with ssid and pwd
#include "secDefine1.h"

#include <RobotSharedDefines.h>

#include "ESP8266WiFi.h"

#include <StreamParser.h>

#include "githash.h"


void setupWifi();
void setup();
void loop();
void heartbeat();

void scanNetworks();

void connectToHome();
void connectToHomeExt();
void connectToBase();
void beTheAP();

void killConnection();

void handleClient(char*);
void handleSerial(char*);
void waitOnRMB(char*);


#endif /* _RobotWifi_ESP07_H_ */
