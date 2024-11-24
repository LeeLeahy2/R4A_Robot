/**********************************************************************
  Telnet_Client.cpp

  Robots-For-All (R4A)
  Telnet client support
  See: https://www.rfc-editor.org/rfc/pdfrfc/rfc854.txt.pdf
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Constructor: Initialize the telnet client object
R4A_TELNET_CLIENT::R4A_TELNET_CLIENT(NetworkClient client,
                                     R4A_TELNET_CLIENT_PROCESS_INPUT processInput,
                                     R4A_TELNET_CONTEXT_CREATE contextCreate,
                                     R4A_TELNET_CONTEXT_DELETE contextDelete)
    : _command{String("")}, _client{client}, _contextCreate{contextCreate},
      _contextData{nullptr}, _contextDelete{contextDelete},
      _processInput{processInput}
{
    // Save the user parameter
    if (_contextCreate)
        _contextCreate(&_client, &_contextData);
}

//*********************************************************************
// Destructor: Cleanup the things allocated in the constructor
R4A_TELNET_CLIENT::~R4A_TELNET_CLIENT()
{
    if (_contextDelete)
        _contextDelete(_contextData);
}

//*********************************************************************
void R4A_TELNET_CLIENT::disconnect()
{
    _client.stop();
}

//*********************************************************************
// Process the incoming telnet client data
bool R4A_TELNET_CLIENT::processInput()
{
    bool connected;
    bool done;

    // Determine if the client is still connected
    done = false;
    connected = _client.connected();
    if (connected)
    {
        // Determine if data is available
        if (_client.available())
        {
            // Process the incoming client data
            while (_client.available())
            {
                // Process the input character
                if (_processInput)
                    done = _processInput(&_client, _contextData);
                else
                    Serial.write(_client.read());

                // Break the client connection when done
                if (done)
                {
                    disconnect();
                    connected = false;
                    break;
                }
            }
        }
    }

    // The client is no longer connected, disconnect on the server side
    else
    {
        disconnect();
        connected = false;
    }

    // Return the connected status
    return connected;
}

//*********************************************************************
// Get the IP address
IPAddress R4A_TELNET_CLIENT::remoteIP()
{
    return _client.remoteIP();
}

//*********************************************************************
// Get the port number
uint16_t R4A_TELNET_CLIENT::remotePort()
{
    return _client.remotePort();
}
