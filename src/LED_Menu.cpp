/**********************************************************************
  LED_Menu.cpp

  Robots-For-All (R4A)
  Multicolor LED menu support
**********************************************************************/

#include "R4A_Robot.h"

//****************************************
// LED menu
//****************************************

const R4A_MENU_ENTRY r4aLEDMenuTable[] =
{
    // Command  menuRoutine         menuParam               HelpRoutine         align   HelpText
    {"c3",      r4aLEDMenuColor3,   (intptr_t)"ll rrggbb",  r4aMenuHelpSuffix,  9,      "Specify the LED ll color rrggbb (RGB in hex)"},    // 0
    {"c4",      r4aLEDMenuColor4,  (intptr_t)"ll wwrrggbb", r4aMenuHelpSuffix,  11,     "Specify the LED ll color wwrrggbb (RGBW in hex)"}, // 1
    {"d",       r4aLEDMenuDisplay,  0,          nullptr,                        0,      "Display the LED status"},                          // 2
    {"i",      r4aLEDMenuIntensity, (intptr_t)"iii",        r4aMenuHelpSuffix,  3,      "Specify the LED intensity iii (0 - 255)"},         // 3
    {"o",       r4aLEDMenuOff,      0,          nullptr,                        0,      "Turn off all LEDs"},                               // 4
    {"x",       nullptr,         R4A_MENU_MAIN, nullptr,                        0,      "Return to the main menu"},                         // 5
};                                                                                                                                          // 6
