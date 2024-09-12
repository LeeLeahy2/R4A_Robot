######################################################################
# makefile
#
# Robots-For-All (R4A)
# Build the example sketch
######################################################################

.ONESHELL:
SHELL=/bin/bash

##########
# Source files
##########

SKETCH=02_Multiple_Menus
ESP32_CHIP=esp32wrover
ESP32_VERSION=2.0.11
PARTITION_FILE_NAME=huge_app

##########
# OS specific paths
##########

ifeq ($(OS),Windows_NT)
#---------
# Windows NT generic paths
#---------

USER_DIRECTORY_PATH=C:\Users\$(USERNAME)\

ARDUINO_PATH=$(USER_DIRECTORY_PATH)Documents\Arduino\
ARDUINO_LIBRARY_PATH=$(ARDUINO_PATH)libraries\
BUILD_PATH=build\
BIN_PATH=$BUILD_PATH)esp32.esp32.$(ESP32_CHIP)\
BOOT_LOADER_PATH=$(HOME_BOARD_PATH)$(ESP32_VERSION)\tools\partitions\
HOME_BOARD_PATH=$(USER_DIRECTORY_PATH)AppData\Local\Arduino15\packages\esp32\
ESP_IDF_PATH=$(HOME_BOARD_PATH)tools\esp32-arduino-libs\
ESPTOOL_PATH=$(USER_DIRECTORY_PATH)\.arduino15\packages\esp32\tools\esptool_py\4.5.1\

# Windows NT utilities
ARDUINO_CLI=~/Arduino/arduino-cli
CLEAR=cls
COPY=copy
DELETE=rmdir /s
DIR_LISTING=dir
TERMINAL_APP=

TERMINAL_PORT=COM3
TERMINAL_PARAMS=

else
#---------
# Linux generic paths
#---------

USER_DIRECTORY_PATH=~/

ARDUINO_PATH=$(USER_DIRECTORY_PATH)Arduino/
ARDUINO_LIBRARY_PATH=$(ARDUINO_PATH)libraries/
BUILD_PATH=build/
BIN_PATH=$(BUILD_PATH)esp32.esp32.$(ESP32_CHIP)/
HOME_BOARD_PATH=$(USER_DIRECTORY_PATH).arduino15/packages/esp32/
BOOT_LOADER_PATH=$(ARDUINO_LIBRARY_PATH)R4A_ESP32/binaries/
ESP_IDF_PATH=$(HOME_BOARD_PATH)tools/esp32-arduino-libs/
ESPTOOL_PATH=$(USER_DIRECTORY_PATH).arduino15/packages/esp32/tools/esptool_py/4.5.1/

# Linux utilities
ARDUINO_CLI=$(USER_DIRECTORY_PATH)Arduino/arduino-cli
CLEAR=clear
COPY=cp
DELETE=rm -Rf
DIR_LISTING=ls
TERMINAL_APP=minicom

TERMINAL_PORT=/dev/ttyUSB0
TERMINAL_PARAMS=-b 115200 -8 -D $(TERMINAL_PORT) < /dev/tty

endif

#---------
# OS Independent
#---------

# Files
BIN_FILE=$(BIN_PATH)$(SKETCH).ino.bin

##########
# Buid all the sources - must be first
##########

EXECUTABLES += $(BIN_FILE)

.PHONY: all

all: $(EXECUTABLES)

.PHONY: Car

Car:	$(BIN_FILE)

########
# Robot Firmware
##########

#DEBUG_LEVEL=debug
DEBUG_LEVEL=none

$(BIN_FILE):	$(SKETCH).ino   *.ino   *.h   makefile
	$(CLEAR)
	echo "----------------------------------------------------------------------"
	$(ARDUINO_CLI)  \
		compile  \
		--fqbn   "esp32:esp32:$(ESP32_CHIP)":DebugLevel=$(DEBUG_LEVEL),PartitionScheme=$(PARTITION_FILE_NAME),FlashMode=qio,FlashFreq=80,UploadSpeed=921600,EraseFlash=none  \
		$<  \
		--build-property build.partitions=$(PARTITION_FILE_NAME)  \
		--build-property upload.maximum_size=3145728  \
		--export-binaries

##########
# Upload the firmware
##########

.PHONY: upload

upload:	$(BIN_FILE)
	python3 $(ESPTOOL_PATH)esptool.py \
		--chip   esp32 \
		--port   $(TERMINAL_PORT) \
		--baud   921600 \
		--before   default_reset \
		--after   hard_reset \
		write_flash \
		--flash_mode dio \
		--flash_freq 80m \
		--flash_size detect \
		--compress \
		 0x1000   $(BOOT_LOADER_PATH)bootloader.bin \
		 0x8000   $(BOOT_LOADER_PATH)$(PARTITION_FILE_NAME).bin \
		 0xe000   $(BOOT_LOADER_PATH)boot_app0.bin \
		0x10000   $<
	$(TERMINAL_APP) $(TERMINAL_PARAMS)

##########
# Open the terminal (tty)
##########

.PHONY: terminal

terminal:
	$(TERMINAL_APP) $(TERMINAL_PARAMS)

########
# Clean the build directory
##########

.PHONY: clean

clean:
	rm -Rf $(BUILD_PATH)
