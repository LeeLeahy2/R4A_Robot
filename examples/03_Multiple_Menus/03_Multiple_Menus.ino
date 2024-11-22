/**********************************************************************
  03_Multiple_Menus.ino

  Robots-For-All (R4A)
  Example telnet server and serial with multiple menus
**********************************************************************/

#include <R4A_Robot.h>

#include "secrets.h"

WiFiMulti wifiMulti;

//****************************************
// Locals
//****************************************

static bool dumpBuffer;
static bool echoCommand;

//****************************************
// Forward routine declarations
//****************************************

bool contextCreate23(NetworkClient * client, void ** contextData);
bool contextCreate24(NetworkClient * client, void ** contextData);
void contextDelete(void * contextData);
void listClients23(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);
void listClients24(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);
bool serialOutput(NetworkClient * client, void * contextData);
void serverInfo23(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);
void serverInfo24(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

//****************************************
// Globals
//****************************************

R4A_TELNET_SERVER telnet23(4,
                           r4aTelnetContextProcessInput,
                           contextCreate23,
                           r4aTelnetContextDelete);
R4A_TELNET_SERVER telnet24(4,
                           r4aTelnetContextProcessInput,
                           contextCreate24,
                           r4aTelnetContextDelete);
R4A_TELNET_SERVER telnet25(4, serialOutput, nullptr, nullptr);

//****************************************
// Menus
//****************************************

enum MENU_TABLE_INDEX
{
    mtiTelnetMenu = R4A_MENU_MAIN + 1,
    mtiCommandMenu,
};

const R4A_MENU_ENTRY telnetMenuTable23[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"List",    listClients23,  0,              nullptr,    0,      "List the telnet clients"}, // 0
    {"Server",  serverInfo23,   0,              nullptr,    0,      "Server info"},             // 1
    {"x",       nullptr,        R4A_MENU_MAIN,  nullptr,    0,      "Return to the main menu"}, // 2
};                                                                                                          // 3
#define TELNET_MENU_23_ENTRIES    sizeof(telnetMenuTable23) / sizeof(telnetMenuTable23[0])

const R4A_MENU_ENTRY telnetMenuTable24[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"List",    listClients24,  0,              nullptr,    0,      "List the telnet clients"}, // 0
    {"Server",  serverInfo24,   0,              nullptr,    0,      "Server info"},             // 1
    {"x",       nullptr,        R4A_MENU_MAIN,  nullptr,    0,      "Return to the main menu"}, // 2
};                                                                                                          // 3
#define TELNET_MENU_24_ENTRIES    sizeof(telnetMenuTable24) / sizeof(telnetMenuTable24[0])

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
const R4A_MENU_ENTRY mainMenuTable23[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"telnet",  nullptr,        mtiTelnetMenu,  nullptr,    0,      "Enter the telnet menu"},
    {"x",       nullptr,        R4A_MENU_NONE,  nullptr,    0,      "Exit the menu system"},
};
#define MAIN_MENU_23_ENTRIES    sizeof(mainMenuTable23) / sizeof(mainMenuTable23[0])

const R4A_MENU_TABLE menuTable23[] =
{
    // menuName         preMenu routine     firstEntry          entryCount
    {"Main Menu 23",    nullptr,            mainMenuTable23,    MAIN_MENU_23_ENTRIES},
    {"Telnet Menu",     nullptr,            telnetMenuTable23,  TELNET_MENU_23_ENTRIES},
};
const int menuTable23Entries = sizeof(menuTable23) / sizeof(menuTable23[0]);

// Main menu
const R4A_MENU_ENTRY mainMenuTable24[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"telnet",  nullptr,        mtiTelnetMenu,  nullptr,    0,      "Enter the telnet menu"},
    {"command", nullptr,        mtiCommandMenu, nullptr,    0,      "Enter the command menu"},
    {"x",       nullptr,        R4A_MENU_NONE,  nullptr,    0,      "Exit the menu system"},
};
#define MAIN_MENU_24_ENTRIES    sizeof(mainMenuTable24) / sizeof(mainMenuTable24[0])

const R4A_MENU_TABLE menuTable24[] =
{
    // menuName         preMenu routine     firstEntry          entryCount
    {"Main Menu 24",    nullptr,            mainMenuTable24,    MAIN_MENU_24_ENTRIES},
    {"Telnet Menu",     nullptr,            telnetMenuTable24,  TELNET_MENU_24_ENTRIES},
    {"Command Menu",    nullptr,            commandMenuTable,   COMMAND_MENU_ENTRIES},
};
const int menuTable24Entries = sizeof(menuTable24) / sizeof(menuTable24[0]);

R4A_MENU serialMenu(menuTable24, menuTable24Entries);

//*********************************************************************
// Entry point for the application
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.printf("%s\r\n", __FILE__);

/*
    // Start the WiFi network
    Serial.println("Connecting to WiFi");
    WiFi.begin(wifiSSID, wifiPassword);
*/

    // Scan WiFi for possible remote APs
    Serial.println("Scanning WiFi ");
    wifiMulti.addAP(wifiSSID1, wifiPassword1);
    wifiMulti.addAP(wifiSSID2, wifiPassword2);

    // Connect to a remote AP if possible
    Serial.println("Connecting Wifi ");
    for (int loops = 10; loops > 0; loops--)
    {
        if (wifiMulti.run() == WL_CONNECTED)
            break;
        else
        {
            Serial.println(loops);
            delay(1000);
        }
    }
    if (wifiMulti.run() != WL_CONNECTED)
    {
        Serial.println("WiFi connect failed");
        delay(1000);
        ESP.restart();
    }

    // Start the telnet server
    telnet23.begin(WiFi.STA.localIP(), 23);
    telnet24.begin(WiFi.STA.localIP(), 24);
    telnet25.begin(WiFi.STA.localIP(), 25);
}

//*********************************************************************
// Idle loop for the application
void loop()
{
    static bool previousConnected23;
    static bool previousConnected24;
    static bool previousConnected25;
    bool wifiConnected;

/*
    // Determine if WiFi is connected
    wifiConnected = (WiFi.status() == WL_CONNECTED);
*/
    // Update the telnet server state
    wifiConnected = (wifiMulti.run() == WL_CONNECTED);

    // Check for NTP updates
    r4aNtpUpdate(wifiConnected);

    // Update the telnet server state
    telnet23.update(wifiConnected);
    if (previousConnected23 != wifiConnected)
    {
        previousConnected23 = wifiConnected;
        if (wifiConnected)
            Serial.printf("Telnet23: %s:%d\r\n",
                          telnet23.ipAddress().toString().c_str(),
                          telnet23.port());
    }

    // Update the telnet server state
    telnet24.update(wifiConnected);
    if (previousConnected24 != wifiConnected)
    {
        previousConnected24 = wifiConnected;
        if (wifiConnected)
            Serial.printf("Telnet24: %s:%d\r\n",
                          telnet24.ipAddress().toString().c_str(),
                          telnet24.port());
    }

    // Update the telnet server state
    telnet25.update(wifiConnected);
    if (previousConnected25 != wifiConnected)
    {
        previousConnected25 = wifiConnected;
        if (wifiConnected)
            Serial.printf("Telnet25: %s:%d\r\n",
                          telnet25.ipAddress().toString().c_str(),
                          telnet25.port());
    }

    // Process commands from the serial port
    r4aSerialMenu(&serialMenu);
}

//*********************************************************************
// Finish creating the network client
// Inputs:
//   client: Address of a NetworkClient object
//   contextData: Buffer to receive the address of an object allocated by
//                this routine
// Outputs:
//   Returns true if the routine was successful and false upon failure.
bool contextCreate23(NetworkClient * client, void ** contextData)
{
    // Return an optional object address to be used as a parameter for
    // r4aTelnetClientProcessInput
    return r4aTelnetContextCreate(client,
                                  menuTable23,
                                  menuTable23Entries,
                                  contextData);
}

//*********************************************************************
// Finish creating the network client
// Inputs:
//   client: Address of a NetworkClient object
//   contextData: Buffer to receive the address of an object allocated by
//                this routine
// Outputs:
//   Returns true if the routine was successful and false upon failure.
bool contextCreate24(NetworkClient * client, void ** contextData)
{
    // Return an optional object address to be used as a parameter for
    // r4aTelnetClientProcessInput
    return r4aTelnetContextCreate(client,
                                  menuTable24,
                                  menuTable24Entries,
                                  contextData);
}

//*********************************************************************
// Display the telnet clients
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void listClients23(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display)
{
    telnet23.listClients(display);
}

//*********************************************************************
// Display the telnet clients
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void listClients24(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display)
{
    telnet24.listClients(display);
}

//*********************************************************************
// Process input from the telnet client
// Inputs:
//   client: Address of a NetworkClient object
//   contextData: Address of object allocated by r4aTelnetClientBegin
// Outputs:
//   Returns true if the client is done (requests exit)
bool serialOutput(NetworkClient * client, void * contextData)
{
    // Copy the output to the serial port
    while (client->available())
        Serial.write(client->read());
    return false;
}

//*********************************************************************
// Display the server information
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void serverInfo23(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display)
{
    telnet23.serverInfo(display);
}

//*********************************************************************
// Display the server information
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void serverInfo24(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display)
{
    telnet24.serverInfo(display);
}
