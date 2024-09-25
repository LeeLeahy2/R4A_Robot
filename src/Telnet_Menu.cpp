/**********************************************************************
  Telnet_Menu.cpp

  Robots-For-All (R4A)
  Telnet menu support
**********************************************************************/

#include "R4A_Robot.h"

//****************************************
// Telnet menu
//****************************************

const R4A_MENU_ENTRY r4aTelnetMenuTable[] =
{
    // Command  menuRoutine             menuParam       HelpRoutine align   HelpText
    {"clients", r4aTelnetMenuClients,   0,              nullptr,    0,      "Display the telnet client list"},  // 0
    {"options", r4aTelnetMenuOptions,   0,              nullptr,    0,      "Display the telnet options"},      // 1
    {"state",   r4aTelnetMenuState,     0,              nullptr,    0,      "Display changes in telnet state"}, // 2
    {"x",       nullptr,                R4A_MENU_MAIN,  nullptr,    0,      "Return to the main menu"},         // 3
};                                                                                                      // 4
