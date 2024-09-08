/**********************************************************************
  Telnet.cpp

  Telnet client and server support
  See: https://www.rfc-editor.org/rfc/pdfrfc/rfc854.txt.pdf
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Display the client list
void R4A_TELNET_SERVER::displayClientList(Print * display)
{
    R4A_TELNET_CLIENT * client;

    // Display the list header
    display->println();
    display->println("Telnet Server Client List");
    client = _clientList;

    // Walk the list of clients
    while (client)
    {
        // Display the client
        display->println("      |");
        display->println("      V");
        display->printf("%p) Client IP Address: %s:%d\r\n",
                        client,
                        client->remoteIP().toString().c_str(),
                        client->remotePort());

        // Get the next client
        client = client->_nextClient;
    }

    // Display the end of the list
    display->println("      |");
    display->println("      V");
    display->println("nullptr - End of List");
    display->println();
}

//*********************************************************************
// Get the telnet port number
// Returns the telnet port number set during initalization
uint16_t R4A_TELNET_SERVER::port()
{
    return _port;
}

//*********************************************************************
// Start and update the telnet server
void R4A_TELNET_SERVER::update(bool wifiConnected)
{
    R4A_TELNET_CLIENT * client;
    bool clientDone;
    bool debugState;
    bool displayOptions;
    R4A_TELNET_CLIENT ** previousClient;

    debugState = _debugState;
    displayOptions = _displayOptions;
    switch (_state)
    {
    case R4A_TELNET_STATE_OFF:
        // Wait until WiFi is available
        if (wifiConnected)
        {
            // Start the server
            _clientList = nullptr;
            _newClient = nullptr;
             WiFiServer(port);
            begin(_port);
            if (debugState)
                Serial.println("Telnet Server: OFF --> ALLOCATE_CLIENT");
            _state = R4A_TELNET_STATE_ALLOCATE_CLIENT;
        }
        break;

    case R4A_TELNET_STATE_ALLOCATE_CLIENT:
        // Check for WiFi failure
        if (!wifiConnected)
        {
            // Wifi has failed
            if (debugState)
                Serial.println("Telnet Server: ALLOCATE_CLIENT --> SHUTDOWN");
            _state = R4A_TELNET_STATE_SHUTDOWN;
        }
        else
        {
            // Wifi still connected to an access point
            // Allocate the next client heap is available
            _newClient = new R4A_TELNET_CLIENT();

            // Display the newClient
            if (debugState)
                Serial.printf("_newClient: %p\r\n", _newClient);

            // Enter listening state when memory is available
            if (_newClient)
            {
                if (debugState)
                    Serial.println("Telnet Server: ALLOCATE_CLIENT --> LISTENING");
                _state = R4A_TELNET_STATE_LISTENING;
            }
        }
        break;

    case R4A_TELNET_STATE_LISTENING:
        if (!wifiConnected)
        {
            // Wifi has failed
            if (debugState)
                Serial.println("Telnet Server: LISTENING --> SHUTDOWN");
            _state = R4A_TELNET_STATE_SHUTDOWN;
        }
        else
        {
            WiFiClient * wifiClient;

            // Wifi still connected to an access point
            // Wait for a client connection
            wifiClient = _newClient;
            *wifiClient = available();
            if (*wifiClient)
            {
                if (debugState)
                    Serial.printf("%p) Client %s:%d connected\r\n",
                                  _newClient,
                                  _newClient->remoteIP().toString().c_str(),
                                  _newClient->remotePort());

                // Display the main menu
                _processCommand(nullptr, _newClient);

                // Add this client to the list of clients
                _newClient->_nextClient = _clientList;
                _clientList = _newClient;
                _newClient = nullptr;

                // Allocate another client
                if (debugState)
                    Serial.println("Telnet Server: LISTENING --> ALLOCATE_CLIENT");
                _state = R4A_TELNET_STATE_ALLOCATE_CLIENT;
            }
        }
        break;

    case R4A_TELNET_STATE_SHUTDOWN:
        // Release the next client
        if (_newClient)
        {
            delete _newClient;
            _newClient = nullptr;
        }

        // Shutdown the telnet server
        stop();
        if (debugState)
            Serial.println("Telnet Server: SHUTDOWN --> OFF");
        _state = R4A_TELNET_STATE_OFF;
        break;
    }

    // Walk the list of telnet clients
    previousClient = &_clientList;
    for (client = _clientList; client != nullptr; client = *previousClient)
    {
        // Check for network failure
        clientDone = false;
        if ((!wifiConnected) || (!client->connected()))
            // Break the client connection
            clientDone = true;
        else
        {
            uint8_t buffer[16];
            String * command;
            uint8_t option;
            uint8_t parameter;

            // Wait for data
            if (client->available())
            {
                // Strip any telnet header
                while (client->peek() == 0xff)
                {
                    // Discard the leading character
                    client->read();

                    // Get the option byte : WILL, WON'T, DO, DON'T
                    // See: https://www.iana.org/assignments/telnet-options/telnet-options.xhtml
                    option = client->read();
                    if (option >= 0xfb)
                    {
                        // Discard the option parameter
                        // See:
                        //   https://www.rfc-editor.org/rfc/pdfrfc/rfc855.txt.pdf, page 3
                        //   https://www.iana.org/assignments/telnet-options/telnet-options.xhtml
                        parameter = client->read();
                        if (parameter == 0xff)
                            parameter = client->read();

                        // Display the option
                        if (displayOptions)
                            Serial.printf("Ignoring option: 0xff 0x%02x 0x%02x\r\n",
                                          option, parameter);
                    }
                }

                // Get a command from this client
                command = r4aReadLine(_echo, &client->_command, (WiFiClient *)client);

                // If a command is available, process it
                if (command)
                {
                    clientDone = _processCommand(command->c_str(), client);
                    client->_command = "";
                }
            }

            // Check the next client
            if (!clientDone)
                previousClient = &client->_nextClient;
        }

        // Check for done or broken client connection
        if (clientDone)
        {
            // Break the client connection
            if (debugState)
                Serial.printf("Client %s:%d stopping\r\n",
                              client->remoteIP().toString().c_str(),
                              client->remotePort());
            client->stop();

            // Remove the client from the list
            *previousClient = client->_nextClient;

            // Free this client
            delete client;
        }
    }
}
