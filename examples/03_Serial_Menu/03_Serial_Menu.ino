/**********************************************************************
  03_Serial_Menu.ino

  Robots-For-All (R4A)
  Example simplifying the serial input
**********************************************************************/

#include <R4A_Robot.h>

#include "secrets.h"

//****************************************
// Forward declarations
//****************************************

bool mainMenu(const char * command, Print * display = &Serial);

//****************************************
// Locals
//****************************************

static R4A_TELNET_SERVER telnet(mainMenu);

//*********************************************************************
// Entry point for the application
void setup()
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println();

    // Start the WiFi network
    WiFi.begin(wifiSSID, wifiPassword);
}

//*********************************************************************
// Idle loop the application
void loop()
{
    static R4A_COMMAND_PROCESSOR serialMenu;
    static bool previousConnected;
    bool wifiConnected;

    // Notify the telnet server of WiFi changes
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    telnet.update(wifiConnected);
    if (previousConnected != wifiConnected)
    {
        previousConnected = wifiConnected;
        if (wifiConnected)
            Serial.printf("Telnet: %s:%d\r\n", WiFi.localIP().toString().c_str(),
                          telnet.port());
    }

    // Process input from the serial port
    r4aSerialMenu(mainMenu);
}

//*********************************************************************
// Telnet menu command processing
bool telnetMenu(const char * command, Print * display)
{
    // Check for a command
    if (command)
    {
        // Check for return to the main menu
        if (r4aStricmp(command, "x") == 0)
            r4aProcessCommand = mainMenu;

        // Check for the clients command
        else if (r4aStricmp(command, "clients") == 0)
            telnet.displayClientList(display);

        // Check for the options command
        else if (r4aStricmp(command, "options") == 0)
            telnet._displayOptions = telnet._displayOptions ? nullptr : &Serial;

        // Check for the state command
        else if (r4aStricmp(command, "state") == 0)
            telnet._debugState = telnet._debugState ? nullptr : &Serial;

        // Display unknown command
        else
            display->printf("Unknown command: %s\r\n", command);
    }

    // Display the menu
    else
    {
        // Telnet state
        display->printf("_debugState: %s\r\n",
                        telnet._debugState ? "Enabled" : "Disabled");
        display->printf("_displayOptions: %s\r\n",
                        telnet._displayOptions ? "Enabled" : "Disabled");

        // Display the menu
        display->println();
        display->println("Telnet Menu");
        display->println("---------------------------------------");
        display->println("clients: Display the telnet client list");
        display->println("options: Display the telnet options");
        display->println("state: Display changes in telnet state");
        display->println("x: Return to the main menu");
        display->println();
    }

    // Not done yet
    return false;
}

//*********************************************************************
// Main menu command processing
bool mainMenu(const char * command, Print * display)
{
    // Check for a command
    if (command)
    {
        // Check for the exit command
        if (r4aStricmp(command, "exit") == 0)
            r4aProcessCommand = nullptr;

        // Check for the help command
        else if (r4aStricmp(command, "help") == 0)
        {
            display->println();
            display->println("Commands");
            display->println("--------------------------------------------");
            display->println("exit: Break the command processor connection");
            display->println("help: Display this list of commands");
            display->println("telnet: Enter the telnet menu");
            display->println();
        }

        // Check for entering the telnet menu
        else if (r4aStricmp(command, "telnet") == 0)
            r4aProcessCommand = telnetMenu;

        // Display unknown command
        else
            display->printf("Unknown command: %s\r\n", command);
    }

    // Display the menu
    else
    {
        display->println();
        display->println("Main Menu");
        display->println("--------------------------------------------");
        display->println("exit: Break the command processor connection");
        display->println("help: Display this list of commands");
        display->println("telnet: Enter the telnet menu");
        display->println();
    }

    // Check for exit command
    return (r4aProcessCommand == nullptr);
}
