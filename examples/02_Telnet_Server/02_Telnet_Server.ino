/**********************************************************************
  02_Telnet_Server.ino

  Robots-For-All (R4A)
  Example simplifying the telnet input
**********************************************************************/

#include <R4A_ESP32.h>

#include "secrets.h"

#define TELNET_PORT             23
#define MAX_TELNET_CLIENTS      4

WiFiMulti wifiMulti;

//****************************************
// Forward routine declarations
//****************************************

bool contextCreate(void ** contextData, NetworkClient * client);
void listClients(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);
void serverInfo(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

//****************************************
// Globals
//****************************************

R4A_TELNET_SERVER telnet(4,
                         r4aTelnetContextProcessInput,
                         contextCreate,
                         r4aTelnetContextDelete);

//****************************************
// Menus
//****************************************

enum MENU_TABLE_INDEX
{
    mtiTelnetMenu = R4A_MENU_MAIN + 1,
};

const R4A_MENU_ENTRY telnetMenuTable[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"List",    listClients,    0,              nullptr,    0,      "List the telnet clients"}, // 0
    {"Server",  serverInfo,     0,              nullptr,    0,      "Server info"},             // 1
    {"x",       nullptr,        R4A_MENU_MAIN,  nullptr,    0,      "Return to the main menu"}, // 2
};                                                                                                          // 3
#define TELNET_MENU_ENTRIES       sizeof(telnetMenuTable) / sizeof(telnetMenuTable[0])

// Main menu
const R4A_MENU_ENTRY mainMenuTable[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"telnet",  nullptr,        mtiTelnetMenu,  nullptr,    0,      "Enter the telnet menu"},
    {"x",       nullptr,        R4A_MENU_NONE,  nullptr,    0,      "Exit the menu system"},
};
#define MAIN_MENU_ENTRIES       sizeof(mainMenuTable) / sizeof(mainMenuTable[0])

const R4A_MENU_TABLE menuTable[] =
{
    // menuName         preMenu routine             firstEntry              entryCount
    {"Main Menu",       nullptr,                    mainMenuTable,          MAIN_MENU_ENTRIES},
    {"Telnet Menu",     nullptr,                    telnetMenuTable,        TELNET_MENU_ENTRIES},
};
const int menuTableEntries = sizeof(menuTable) / sizeof(menuTable[0]);

//*********************************************************************
// Entry point for the application
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.printf("%s\r\n", __FILE__);

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
    telnet.begin(WiFi.STA.localIP(), TELNET_PORT);

    // Display the IP address
    Serial.println("");
    Serial.printf("WiFi: %s:%d\r\n", WiFi.localIP().toString().c_str(), TELNET_PORT);
}

//*********************************************************************
// Idle loop for the application
void loop()
{
    telnet.update(wifiMulti.run() == WL_CONNECTED);
}

//*********************************************************************
// Finish creating the network client
// Inputs:
//   contextData: Buffer to receive the address of an object allocated by
//                this routine
//   client: Address of a NetworkClient object
// Outputs:
//   Returns true if the routine was successful and false upon failure.
bool contextCreate(void ** contextData, NetworkClient * client)
{
    // Return an optional object address to be used as a parameter for
    // r4aTelnetClientProcessInput
    return r4aTelnetContextCreate(contextData, client, menuTable, menuTableEntries);
}

//*********************************************************************
// Display the telnet clients
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void listClients(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display)
{
    telnet.listClients(display);
}

//*********************************************************************
// Display the server information
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void serverInfo(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display)
{
    telnet.serverInfo(display);
}
