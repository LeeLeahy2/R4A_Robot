#!/bin/bash
#
# check.sh
#    Script to verify that the code still builds successfully.  This
#    script stops execution upon error when a build fails.
########################################################################
set -e
#set -o verbose
#set -o xtrace

# Start fresh
git reset --hard --quiet HEAD

# Select the initial example directory
pushd   ../examples/01_Serial_Menu

# Serial Menu
make clean
make
make clean

# Telnet Server
cd ../02_Telnet_Server
make clean
make
make clean

# Multiple Menus
cd ../03_Multiple_Menus
make clean
make
make clean

# NTP
cd ../04_NTP
make clean
make
make clean

# Bluetooth Menu
cd ../05_Bluetooth_Menu
make clean
make
make clean

# Restore the source directory
popd
