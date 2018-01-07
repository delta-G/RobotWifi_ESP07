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

#include "StreamParser.h"


void StreamParser::run(){

	if(in->available()){
		char c = in->read();

		if(c == sop){
			receiving = true;
			index = 0;
			buffer[0] = 0;
		}
		if (receiving){
			buffer[index] = c;
			buffer[++index] = 0;
			if(index >= BUFFER_SIZE){
				index--;
			}
			if(c == eop){
				receiving = false;
				callback(buffer);
			}
		}
	}
}


void StreamParser::setCallback(void (*aCall)(char*)){
	callback = aCall;
}


