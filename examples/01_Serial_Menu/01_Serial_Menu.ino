/**********************************************************************
  01_Serial_Menu.ino

  Robots-For-All (R4A)
  Example simplifying the serial input
**********************************************************************/

#include <R4A_Robot.h>

//****************************************
// Menus
//****************************************

// Main menu
const R4A_MENU_ENTRY mainMenuTable[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"exit",    nullptr,        R4A_MENU_NONE,  nullptr,    0,      "Exit the menu system"},
};
#define MAIN_MENU_ENTRIES       sizeof(mainMenuTable) / sizeof(mainMenuTable[0])

const R4A_MENU_TABLE menuTable[] =
{
    // menuName         preMenu routine             firstEntry              entryCount
    {"Main Menu",       nullptr,                    mainMenuTable,          MAIN_MENU_ENTRIES},
};
const int menuTableEntries = sizeof(menuTable) / sizeof(menuTable[0]);

//****************************************
// Locals
//****************************************

R4A_MENU serialMenu(menuTable, menuTableEntries);

//*********************************************************************
// Entry point for the application
void setup()
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println();
    Serial.println("Example 01_Serial_Menu");
}

//*********************************************************************
// Idle loop the application
void loop()
{
    // Process the serial commands
    r4aSerialMenu(&serialMenu);
}
