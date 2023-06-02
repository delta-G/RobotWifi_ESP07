#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

#ARDUINO_DIR = /home/david/.arduino15
#ALTERNATE_CORE_PATH = /home/david/.arduino15/packages/esp8266/hardware/esp8266/3.1.2
#ARDMK_DIR = /usr/share/arduino
#AVR_TOOLS_DIR = /usr
#
#USER_LIB_PATH = /home/david/Arduino/libraries
#ARDUINO_PLATFORM_LIB_PATH = /home/david/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries
#
#
#TARGET = RobotWifi_ESP07.cpp
#
#BOARD_TAG	= generic
#
#BOARDS_TXT        = /home/david/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/boards.txt
#
#ARDUINO_PORT = /dev/ttyUSB0  # change this to the port used by your board


$(shell ./preCompile.sh > /dev/null)


SKETCH = /home/david/robot/RobotWifi_ESP07/RobotWifi_ESP07.cpp

CHIP = esp8266

BOARD = generic

UPLOAD_PORT = /dev/ttyUSB0

VERBOSE = 1

LIBS = SPI 

EXCLUDE_DIRS = /home/david/Arduino/libraries/RF24|/home/david/Arduino/libraries/RobotLibrariesGit

BUILD_EXTRA_FLAGS += -DARDUINO_BOARD_ID=\"generic\"

include /home/david/bin/makeEspArduino/makeEspArduino.mk