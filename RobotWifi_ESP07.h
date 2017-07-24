#ifndef _RobotWifi_ESP07_H_
#define _RobotWifi_ESP07_H_
#include "Arduino.h"
// includes file not in repository with ssid and pwd
#include "secDefine1.h"

#include <RobotSharedDefines.h>

#include "ESP8266WiFi.h"

void setupWifi();
void setup();
void loop();
void heartbeat();
#endif /* _RobotWifi_ESP07_H_ */
