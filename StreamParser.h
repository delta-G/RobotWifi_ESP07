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

#ifndef STREAMPARSER_H_
#define STREAMPARSER_H_

#include "Arduino.h"
#include "ESP8266WiFi.h"

#define BUFFER_SIZE 100



class StreamParser {

private:
	char buffer[BUFFER_SIZE];
	int index;

	Stream* in;
	char sop;
	char eop;

	void (*callback)(char*);

	boolean receiving;

	StreamParser(Stream* aIn, char aSop, char aEop, void(*aCallback)(char*)):in(aIn), sop(aSop), eop(aEop), callback(aCallback), receiving(false), index(0){};
	void run();

};



#endif /* STREAMPARSER_H_ */
