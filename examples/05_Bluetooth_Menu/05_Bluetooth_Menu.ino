/**********************************************************************
  05_Bluetooth_Menu.ino

  Robots-For-All (R4A)
  Example simplifying the serial input
**********************************************************************/

#include <R4A_Robot.h>

#define BLUETOOTH_NAME      "Robot"

//****************************************
// Forward routine declarations
//****************************************

void btAddress(const R4A_MENU_ENTRY * menuEntry = nullptr, const char * command = nullptr, Print * display = &Serial);

//****************************************
// Menus
//****************************************

// Main menu
const R4A_MENU_ENTRY mainMenuTable[] =
{
    // Command  menuRoutine     menuParam       HelpRoutine align   HelpText
    {"addr",    btAddress,      0,              nullptr,    0,      "Display the bluetooth address"},
    {"x",       nullptr,        R4A_MENU_NONE,  nullptr,    0,      "Exit the menu system"},
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

BluetoothSerial btSerial;
R4A_MENU bluetoothMenu(menuTable, menuTableEntries);

//*********************************************************************
// Entry point for the application
void setup()
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println();
    Serial.printf("%s\r\n", __FILE__);

    // Initialize the bluetooth serial port
    if (btSerial.begin(BLUETOOTH_NAME) == false)
        r4aReportFatalError("Failed to establish Bluetooth serial port!");
    btAddress();
}

//*********************************************************************
// Idle loop for the application
void loop()
{
    bool connected;
    bool done;
    static bool previouslyConnected;

    // Determine if a new connection was established
    connected = btSerial.hasClient();
    if (previouslyConnected != connected)
    {
        previouslyConnected = connected;

        // Display the initial menu upon connection
        if (connected)
        {
            bluetoothMenu.process(nullptr, &btSerial);
            previouslyConnected = connected;
        }
    }

    // Process the serial commands
    if (connected)
    {
        done = r4aBluetoothMenu(&bluetoothMenu, &btSerial);
        if (done)
            btSerial.disconnect();
    }
}

//*********************************************************************
// Display the Bluetooth address
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void btAddress(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display)
{
    uint8_t mac[6];

    btSerial.getBtAddress(mac);
    display->printf("Bluetooth: %s (%02x:%02x:%02x:%02x:%02x:%02x)\r\n",
                    BLUETOOTH_NAME, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
