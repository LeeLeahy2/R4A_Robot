/**********************************************************************
  03_Multiple_Menus.ino

  Robots-For-All (R4A)
  Example telnet server and serial with multiple menus
**********************************************************************/

#include <R4A_Robot.h>

#include "secrets.h"

//****************************************
// Locals
//****************************************

static bool dumpBuffer;
static bool echoCommand;

//****************************************
// Menus
//****************************************

enum MENU_TABLE_INDEX
{
    mtiCommandMenu = R4A_MENU_MAIN + 1,
    mtiTelnetMenu,
};

// Command menu
const R4A_MENU_ENTRY commandMenuTable[] =
{
    // Command  menuRoutine         menuParam               HelpRoutine         align   HelpText
    {"dump",    r4aMenuBoolToggle,  (intptr_t)&dumpBuffer,  r4aMenuBoolHelp,    0,      "Toggle command buffer dump"},
    {"echo",    r4aMenuBoolToggle,  (intptr_t)&echoCommand, r4aMenuBoolHelp,    0,      "Toggle command display"},
    {"x",       nullptr,            R4A_MENU_MAIN,          nullptr,            0,      "Return to the main menu"},
};
#define COMMAND_MENU_ENTRIES    sizeof(commandMenuTable) / sizeof(commandMenuTable[0])

// Main menu
const R4A_MENU_ENTRY mainMenuTable[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"command", nullptr,        mtiCommandMenu, nullptr,    0,      "Enter the command menu"},
    {"telnet",  nullptr,        mtiTelnetMenu,  nullptr,    0,      "Enter the telnet menu"},
    {"exit",    nullptr,        R4A_MENU_NONE,  nullptr,    0,      "Exit the menu system"},
};
#define MAIN_MENU_ENTRIES       sizeof(mainMenuTable) / sizeof(mainMenuTable[0])

const R4A_MENU_TABLE menuTable[] =
{
    // menuName         preMenu routine             firstEntry          entryCount
    {"Main Menu",       nullptr,                    mainMenuTable,      MAIN_MENU_ENTRIES},
    {"Command Menu",    nullptr,                    commandMenuTable,   COMMAND_MENU_ENTRIES},
    {"Telnet Menu",     r4aTelnetMenuStateDisplay,  r4aTelnetMenuTable, R4A_TELNET_MENU_ENTRIES},
};
const int menuTableEntries = sizeof(menuTable) / sizeof(menuTable[0]);

R4A_MENU serialMenu(menuTable, menuTableEntries);
R4A_TELNET_SERVER telnet(menuTable, menuTableEntries);

//*********************************************************************
// Entry point for the application
void setup()
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println();
    Serial.println("Example 03_Multiple_Menus");

    // Adjust the serial menu options
    serialMenu._blankLineBeforePreMenu = false;
    serialMenu._blankLineBeforeMenuHeader = false;
    serialMenu._blankLineAfterMenuHeader = true;
    serialMenu._alignCommands = false;
    serialMenu._blankLineAfterMenu = true;

    // Start the WiFi network
    Serial.println("Connecting to WiFi");
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

    // Process commands from the serial port
    r4aSerialMenu(&serialMenu);
}
