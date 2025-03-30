/*------------------------------------------------------------------------------
NTRIP_Client_Menu.cpp

  Robots-For-All (R4A)
  Menu for the NTRIP client
------------------------------------------------------------------------------*/

#include "R4A_Robot.h"

//*********************************************************************
void r4aNtripClientMenuState(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    display->print("NTRIP Client state: ");
    r4aNtripClientPrintStateSummary(display);
    display->println();

}

//*********************************************************************
void r4aNtripClientMenuStatus(const R4A_MENU_ENTRY * menuEntry,
                              const char * command,
                              Print * display)
{
    r4aNtripClientPrintStatus(display);
}

//****************************************
// NTRIP client menu
//****************************************

const R4A_MENU_ENTRY r4aNtripClientMenuTable[] =
{
// Command      menuRoutine             menuParam                                   HelpRoutine         align   HelpText
    {"c",       r4aMenuBoolToggle,      (intptr_t)&r4aNtripClientForcedShutdown,    r4aMenuBoolHelp,    0,      "Clear the forced error"},      // 0
    {"d",       r4aMenuBoolToggle,      (intptr_t)&r4aNtripClientDebugRtcm,         r4aMenuBoolHelp,    0,      "Toggle NTRIP debug RTCM"},     // 1
    {"e",       r4aMenuBoolToggle,      (intptr_t)&r4aNtripClientEnable,            r4aMenuBoolHelp,    0,      "Toggle NTRIP client enable"},  // 2
    {"s",       r4aMenuBoolToggle,      (intptr_t)&r4aNtripClientDebugState,        r4aMenuBoolHelp,    0,      "Toggle NTRIP state display"},  // 3
    {"state",   r4aNtripClientMenuState,    0,                                      nullptr,            0,      "Display NTRIP client state"},  // 4
    {"status",  r4aNtripClientMenuStatus,   0,                                      nullptr,            0,      "Display NTRIP client status"}, // 5
    {"x",       nullptr,                R4A_MENU_MAIN,                              nullptr,            0,      "Return to the main menu"},     // 6
};                                                                                                                                      // 7
