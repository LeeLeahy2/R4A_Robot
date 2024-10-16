/*------------------------------------------------------------------------------
NTRIP_Client_Menu.cpp

  Robots-For-All (R4A)
  Menu for the NTRIP client
------------------------------------------------------------------------------*/

#include "R4A_Robot.h"

//****************************************
// NTRIP client menu
//****************************************

const R4A_MENU_ENTRY r4aNtripClientMenuTable[] =
{
// Command  menuRoutine         menuParam                                   HelpRoutine         align   HelpText
    {"c",   r4aMenuBoolToggle,  (intptr_t)&r4aNtripClientForcedShutdown,    r4aMenuBoolHelp,    0,      "Clear the forced error"},      // 0
    {"d",   r4aMenuBoolToggle,  (intptr_t)&r4aNtripClientDebugRtcm,         r4aMenuBoolHelp,    0,      "Toggle NTRIP debug RTCM"},     // 1
    {"e",   r4aMenuBoolToggle,  (intptr_t)&r4aNtripClientEnable,            r4aMenuBoolHelp,    0,      "Toggle NTRIP client enable"},  // 2
    {"s",   r4aMenuBoolToggle,  (intptr_t)&r4aNtripClientDebugState,        r4aMenuBoolHelp,    0,      "Toggle NTRIP state display"},  // 3
    {"x",   nullptr,            R4A_MENU_MAIN,                              nullptr,            0,      "Return to the main menu"},     // 4
};                                                                                                                                      // 5
