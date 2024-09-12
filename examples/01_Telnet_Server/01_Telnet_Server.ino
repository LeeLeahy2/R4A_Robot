/**********************************************************************
  Telnet_Server.ino

  Robots-For-All (R4A)
  Example telnet server, echos the user's commands
**********************************************************************/

#include <R4A_Robot.h>

#include "secrets.h"

//****************************************
// Forward declarations
//****************************************

bool processCommand(const char * command, Print * display = &Serial);

//****************************************
// Locals
//****************************************

static bool dumpBuffer;
static bool echoCommand;
static R4A_TELNET_SERVER telnet(processCommand);

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
    {
        bool done;
        const char * command;
        String * line;
        static String serialBuffer;

        line = r4aReadLine(true, &serialBuffer, &Serial);
        if (line)
        {
            // Dump the command buffer
            command = line->c_str();
            if (dumpBuffer)
                r4aDumpBuffer(0, (const uint8_t *)command, strlen(command) + 1);
            if (echoCommand)
                Serial.printf("Command: %s\r\n", command);

            // Process the command
            done = false;
            if (strlen(command))
            {
                done = processCommand(command);
                serialBuffer = "";
            }

            // Display the menu
            if (!done)
                processCommand(nullptr);
        }
    }
}

//*********************************************************************
// Command processing
bool processCommand(const char * command, Print * display)
{
    bool done;

    // Check for a command
    done = false;
    if (command)
    {
        // Check for the exit command
        if (r4aStricmp(command, "exit") == 0)
            done = true;

        // Check for the d (display) command
        else if (r4aStricmp(command, "d") == 0)
            telnet.displayClientList(display);

        // Check for the dump command
        else if (r4aStricmp(command, "dump") == 0)
            dumpBuffer ^= 1;

        // Check for the echo command
        else if (r4aStricmp(command, "echo") == 0)
            echoCommand ^= 1;

        // Check for the options command
        else if (r4aStricmp(command, "options") == 0)
            telnet._displayOptions = telnet._displayOptions ? nullptr : &Serial;

        // Check for the state command
        else if (r4aStricmp(command, "state") == 0)
            telnet._debugState = telnet._debugState ? nullptr : &Serial;

        // Check for the telnet command
        else if (r4aStricmp(command, "telnet") == 0)
        {
            display->printf("_debugState: %s\r\n",
                            telnet._debugState ? "Enabled" : "Disabled");
            display->printf("_displayOptions: %s\r\n",
                            telnet._displayOptions ? "Enabled" : "Disabled");
        }

        // Display unknown command
        else
            display->printf("Unknown command: %s\r\n", command);
    }
    else
    {
        display->println();
        display->println("Menu");
        display->println("--------------------------------------------");
        display->println("d: Display the client list");
        display->println("dump: Toggle command buffer dump");
        display->println("echo: Toggle command display");
        display->println("exit: Break the command processor connection");
        display->println("options: Display the telnet options");
        display->println("state: Display changes in telnet state");
        display->println("telnet: Display the telnet debug settings");
        display->println();
    }

    // Check for exit command
    return done;
}
