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
                                     R4A_TELNET_CONTEXT_DELETE contextDelete)
    : _activeClients{0}, _clients{nullptr}, _contextCreate{contextCreate},
      _contextDelete{contextDelete}, _ipAddress{IPAddress((uint32_t)0)},
      _maxClients{maxClients}, _port{0}, _processInput{processInput},
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
        free(_clients);
        _clients = nullptr;
    }
}

//*********************************************************************
// Initialize the telnet server
// Returns true following successful server initialization and false
// upon failure.
bool R4A_TELNET_SERVER::begin(IPAddress ipAddress, uint16_t port)
{
    size_t length;

    log_v("R4A_TELNET_SERVER::begin called");

    // Determine if the server was already initialized
    while (_server == nullptr)
    {
        // Save the port for display
        _ipAddress = ipAddress;
        _port = port;

        // Allocate the client list
        length = _maxClients * sizeof(R4A_TELNET_CLIENT *);
        log_v("Telnet Server: Allocating R4A_TELNET_CLIENT array, %d bytes", length);
        _clients = (R4A_TELNET_CLIENT **)malloc(length);
        if (!_clients)
        {
            Serial.printf("ERROR: Failed to allocate R4A_TELNET_CLIENT list!\r\n");
            break;
        }
        memset(_clients, 0, length);

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
    // Done with the client list
    if (_clients)
    {
        // Done with each of the clients
        for (int i = 0; i < _maxClients; i++)
        {
            if (_clients[i])
                closeClient(i);
        }
    }

    // Done with the server
    if (_server)
    {
        log_v("Telnet Server: Deleting server %p", _server);
        delete _server;
        _server = nullptr;
        _ipAddress = IPAddress{(uint32_t)0};
        _port = 0;
    }
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
    client = _server->accept();

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
void R4A_TELNET_SERVER::update(bool connected)
{
    R4A_TELNET_CLIENT * client;
    uint8_t i;

    // Determine if the network is still working
    if (connected)
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

    // The network connection is broken
    else if (_activeClients)
    {
        // Disconnect all of the client connections
        for (i = 0; i < _maxClients; i++)
        {
            client = _clients[i];
            if (client)
            {
                // Disconnected client, free this slot
                client->disconnect();
                closeClient(i);
            }
        }
    }
}
