/**********************************************************************
  Telnet_Server.cpp

  Robots-For-All (R4A)
  Telnet server support
  See: https://www.rfc-editor.org/rfc/pdfrfc/rfc854.txt.pdf
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Constructor: Create the telnet server object
R4A_TELNET_SERVER::R4A_TELNET_SERVER(uint16_t maxClients,
                                     R4A_TELNET_CLIENT_PROCESS_INPUT processInput,
                                     R4A_TELNET_CONTEXT_CREATE contextCreate,
                                     R4A_TELNET_CONTEXT_DELETE contextDelete,
                                     uint16_t port
                                     )
    : _activeClients{0}, _clients{nullptr}, _contextCreate{contextCreate},
      _contextDelete{contextDelete}, _ipAddress{IPAddress((uint32_t)0)},
      _maxClients{maxClients}, _port{port}, _processInput{processInput},
      _server{nullptr}
{
}

//*********************************************************************
// Destructor: Cleanup the things allocated in the constructor
R4A_TELNET_SERVER::~R4A_TELNET_SERVER()
{
    // Done with the server
    end();

    // Done with the client list
    if (_clients)
    {
        r4aFree((void *)_clients, "Telnet client array (_clients)");
        _clients = nullptr;
    }
}

//*********************************************************************
// Initialize the telnet server
// Returns true following successful server initialization and false
// upon failure.
bool R4A_TELNET_SERVER::begin(IPAddress ipAddress)
{
    size_t length;

    log_v("R4A_TELNET_SERVER::begin called");

    // Determine if the server was already initialized
    while (_server == nullptr)
    {
        // Save the port for display
        _ipAddress = ipAddress;

        // Allocate the client list
        length = _maxClients * sizeof(R4A_TELNET_CLIENT *);
        log_v("Telnet Server: Allocating R4A_TELNET_CLIENT array, %d bytes", length);
        _clients = (R4A_TELNET_CLIENT **)r4aMalloc(length, "Telnet client array (_clients)");
        if (!_clients)
        {
            Serial.printf("ERROR: Failed to allocate R4A_TELNET_CLIENT list!\r\n");
            break;
        }
        memset(_clients, 0, length);
        _activeClients = 0;

        // Allocate the network object
        log_v("Telnet Server: Allocating NetworkServer object");
        _server = new NetworkServer(_port);
        log_v("Telnet Server: Allocated NetworkServer object %p", _server);
        if (!_server)
        {
            Serial.printf("ERROR: Failed to allocate NetworkServer object!\r\n");
            break;
        }

        // Initialize the network server object
        log_v("TelnetServer: Initializing the NetworkServer");
        _server->begin();
        _server->setNoDelay(true);
        break;
    }

    // Free the client array upon failure
    if (_server == nullptr)
    {
        r4aFree((void *)_clients, "Telnet client array (_clients)");
        _clients = nullptr;
    }

    // Notify the caller of the initialization status
    log_v("R4A_TELNET_SERVER::begin returning %s", _server ? "true" : "false");
    return (_server != nullptr);
}

//*********************************************************************
// Close the client specified by index i
void R4A_TELNET_SERVER::closeClient(uint16_t i)
{
    if (_clients && _clients[i])
    {
        log_v("Telnet Server: Deleting client %d (%p)", i, _clients[i]);
        _activeClients -= 1;
        delete _clients[i];
        _clients[i] = nullptr;
    }
}

//*********************************************************************
// Restore state to just after constructor execution
void R4A_TELNET_SERVER::end()
{
    R4A_TELNET_CLIENT * client;
    int i;

    // Verify that the client array was allocated
    if (_clients)
    {
        // Disconnect all of the client connections
        for (i = 0; i < _maxClients; i++)
        {
            client = _clients[i];
            if (client)
            {
                // Break the connection with the client
                client->disconnect();

                // Free this client slot
                closeClient(i);
            }
        }
    }

    // No more clients
    _activeClients = 0;

    // Done with the server
    if (_server)
    {
        log_v("Telnet Server: Deleting server %p", _server);
        delete _server;
        _server = nullptr;
        _ipAddress = IPAddress{(uint32_t)0};
    }
}

//*********************************************************************
// Determine if the telnet server has any clients
// Outputs:
//   Returns true if one or more clients are connected.
bool R4A_TELNET_SERVER::hasClient()
{
    R4A_TELNET_CLIENT * client;

    for (int i = 0; i < _maxClients; i++)
    {
        if (_clients)
        {
            client = _clients[i];
            if (client && client->isConnected())
                return true;
        }
    }
    return false;
}

//*********************************************************************
// Get the IP address
IPAddress R4A_TELNET_SERVER::ipAddress(void)
{
    return _ipAddress;
}

//*********************************************************************
// List the clients
void R4A_TELNET_SERVER::listClients(Print * display)
{
    R4A_TELNET_CLIENT * client;

    for (int i = 0; i < _maxClients; i++)
    {
        client = _clients[i];
        if (client)
        {
            display->printf("%d: %s:%d\r\n",
                            i,
                            client->remoteIP().toString().c_str(),
                            client->remotePort());
        }
        else
            display->printf("%d: No client\r\n", i);
    }
}

//*********************************************************************
// Create a new telnet client
void R4A_TELNET_SERVER::newClient()
{
    NetworkClient client;
    uint16_t i;

    // Get the remote client connection
    log_v("Telent Server: Calling accept");
    client = _server->accept();
    if (client)
    {
        // Look for a free slot
        for (i = 0; i < _maxClients; i++)
        {
            // If a free slot is available, allocate a new client
            if (!_clients[i])
            {
                // Fill the slot with the telnet client
                _clients[i] = new R4A_TELNET_CLIENT(client,
                                                    _processInput,
                                                    _contextCreate,
                                                    _contextDelete);
                if (_clients[i])
                    _activeClients += 1;
                else
                    client.stop();
                break;
            }
        }

        // If no free slots then reject incoming server connection request
        if (i >= _maxClients)
            client.stop();
    }
}

//*********************************************************************
// Get the port number
uint16_t R4A_TELNET_SERVER::port()
{
    return _port;
}

//*********************************************************************
// Display the server information
void R4A_TELNET_SERVER::serverInfo(Print * display)
{
    display->printf("Server: %s:%d\r\n",
                    _ipAddress.toString().c_str(),
                    _port);
}

//*********************************************************************
// Update the server state
void R4A_TELNET_SERVER::update(bool telnetEnable, bool wifiStaConnected)
{
    R4A_TELNET_CLIENT * client;
    uint8_t i;

    switch (_state)
    {
    case TELNET_STATE_OFF:
        // Determine if telnet is starting
        if (telnetEnable)
            _state = TELNET_STATE_WAIT_FOR_NETWORK;
        break;

    case TELNET_STATE_WAIT_FOR_NETWORK:
        // Determine if telnet is being stopped
        if (telnetEnable == false)
            _state = TELNET_STATE_OFF;

        // Determine if the network is available
        else if (wifiStaConnected)
        {
            // The network is available, start the telnet server
            begin(WiFi.STA.localIP());
            Serial.printf("Telnet: %s:%d\r\n", WiFi.localIP().toString().c_str(),
                          _port);
            _state = TELNET_STATE_RUNNING;
        }
        break;

    case TELNET_STATE_RUNNING:
        // Determine if the network has failed or telnet is being stopped
        if ((wifiStaConnected == false) || (telnetEnable == false))
        {
            // Stop the telnet server
            end();
            _state = TELNET_STATE_WAIT_FOR_NETWORK;
        }
        else
        {
            // Check if there are any new clients
            if (_server && _server->hasClient())
                newClient();

            // Check clients for data
            for (i = 0; i < _maxClients; i++)
            {
                client = _clients[i];
                if (client)
                {
                    // Process any incoming data, return the connection status
                    if (!client->processInput())

                        // Broken connection, free this slot
                        closeClient(i);
                }
            }
        }
        break;
    }
}

//*********************************************************************
// Write to all clients
// Inputs:
//   data: Data byte to write to the clients
// Outputs:
//   Returns the number of bytes written
size_t R4A_TELNET_SERVER::write(uint8_t data)
{
    R4A_TELNET_CLIENT * client;

    for (int i = 0; i < _maxClients; i++)
    {
        client = _clients[i];
        if (client)
        {
            // Output the data if possible
            if (client->isConnected())
                client->write(data);
        }
    }
    return 1;
}

//*********************************************************************
// Write to all clients
// Inputs:
//   data: Address of the data to write to the clients
//   length: Number of data bytes to write
// Outputs:
//   Returns the number of bytes written
size_t R4A_TELNET_SERVER::write(const uint8_t * data, size_t length)
{
    R4A_TELNET_CLIENT * client;

    for (int i = 0; i < _maxClients; i++)
    {
        client = _clients[i];
        if (client)
        {
            // Output the data if possible
            if (client->isConnected())
                client->write(data, length);
        }
    }
    return length;
}
