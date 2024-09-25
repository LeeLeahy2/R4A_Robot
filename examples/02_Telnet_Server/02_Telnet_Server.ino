/**********************************************************************
  02_Telnet_Server.ino

  Robots-For-All (R4A)
  Example simplifying the telnet input
**********************************************************************/

#include <R4A_Robot.h>

#include "secrets.h"

//****************************************
// Menus
//****************************************

enum MENU_TABLE_INDEX
{
    mtiTelnetMenu = R4A_MENU_MAIN + 1,
};

// Main menu
const R4A_MENU_ENTRY mainMenuTable[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"telnet",  nullptr,        mtiTelnetMenu,  nullptr,    0,      "Enter the telnet menu"},
    {"exit",    nullptr,        R4A_MENU_NONE,  nullptr,    0,      "Exit the menu system"},
};
#define MAIN_MENU_ENTRIES       sizeof(mainMenuTable) / sizeof(mainMenuTable[0])

const R4A_MENU_TABLE menuTable[] =
{
    // menuName         preMenu routine             firstEntry              entryCount
    {"Main Menu",       nullptr,                    mainMenuTable,          MAIN_MENU_ENTRIES},
    {"Telnet Menu",     r4aTelnetMenuStateDisplay,  r4aTelnetMenuTable,     R4A_TELNET_MENU_ENTRIES},
};
const int menuTableEntries = sizeof(menuTable) / sizeof(menuTable[0]);

//****************************************
// Locals
//****************************************

R4A_TELNET_SERVER telnet(menuTable, menuTableEntries);

//*********************************************************************
// Entry point for the application
void setup()
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println();
    Serial.println("Example 02_Telnet_Server");

    // Start the WiFi network
    WiFi.begin(wifiSSID, wifiPassword);
}

//*********************************************************************
// Idle loop the application
void loop()
{
    static bool previousConnected;
    bool wifiConnected;

    // Determine if WiFi is connected
    wifiConnected = (WiFi.status() == WL_CONNECTED);

    // Check for NTP updates
    r4aNtpUpdate(wifiConnected);

    // Notify the telnet server of WiFi changes
    telnet.update(wifiConnected);
    if (previousConnected != wifiConnected)
    {
        previousConnected = wifiConnected;
        if (wifiConnected)
            Serial.printf("Telnet: %s:%d\r\n", WiFi.localIP().toString().c_str(),
                          telnet.port());
    }
}
