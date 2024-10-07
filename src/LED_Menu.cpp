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
    // Command  menuRoutine         menuParam   HelpRoutine align   HelpText
    {"c3",      r4aLEDMenuColor3,   0,          r4aLEDMenuHelpllcccc,   12,     "Specify the LED ll color cccccc (RGB in hex)"},    // 0
    {"c4",      r4aLEDMenuColor4,   0,          r4aLEDMenuHelpllcccc,   12,     "Specify the LED ll color cccccccc (RGBW in hex)"}, // 1
    {"d",       r4aLEDMenuDisplay,  0,          nullptr,                0,      "Display the LED status"},
    {"i",      r4aLEDMenuIntensity, 0,          r4aLEDMenuHelpiii,      4,      "Specify the LED intensity iii (0 - 255)"},         // 2
    {"o",       r4aLEDMenuOff,      0,          nullptr,                0,      "Turn off all LEDs"},                                           // 3
    {"x",       nullptr,         R4A_MENU_MAIN, nullptr,                0,      "Return to the main menu"},                                     // 4
};                                                                                                                                  // 5
