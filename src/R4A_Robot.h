/**********************************************************************
  R4A_Robot.h

  Declare the robot support
**********************************************************************/

#ifndef __R4A_ROBOT_H__
#define __R4A_ROBOT_H__

#include "R4A_Common.h"         // Robots-For-All common support

#include <WiFiServer.h>         // Built-in

// Process the line received via telnet
// Inputs:
//   command: Zero terminated array of characters
//   display: Address of Print object for output
// Outputs:
//   Returns true if the telnet connection should be broken and false otherwise
typedef bool (* R4A_COMMAND_PROCESSOR)(const char * command,
                                       Print * display);

// Support sub-menu processing by changing this value
extern volatile R4A_COMMAND_PROCESSOR r4aProcessCommand;

// Forward declaration
class R4A_TELNET_SERVER;

//*********************************************************************
// Telnet client
class R4A_TELNET_CLIENT : public WiFiClient
{
private:
    R4A_TELNET_CLIENT * _nextClient; // Next client in the server's client list
    String _command; // User command received via telnet
    R4A_COMMAND_PROCESSOR _processCommand; // Routine to process the commands

    // Allow the server to access the command string
    friend R4A_TELNET_SERVER;
};

//*********************************************************************
// Telnet server
class R4A_TELNET_SERVER : WiFiServer
{
private:
    enum R4A_TELNET_SERVER_STATE
    {
        R4A_TELNET_STATE_OFF = 0,
        R4A_TELNET_STATE_ALLOCATE_CLIENT,
        R4A_TELNET_STATE_LISTENING,
        R4A_TELNET_STATE_SHUTDOWN,
    };
    uint8_t _state; // Telnet server state
    uint16_t _port; // Port number for the telnet server
    bool _echo;
    R4A_COMMAND_PROCESSOR _processCommand; // Routine to process the commands
    R4A_TELNET_CLIENT * _newClient; // New client object, ready for listen call
    R4A_TELNET_CLIENT * _clientList; // Singlely linked list of telnet clients

public:

    Print * _debugState; // Address of Print object to display telnet server state changes
    Print * _displayOptions; // Address of Print object to display telnet options

    // Constructor
    R4A_TELNET_SERVER(R4A_COMMAND_PROCESSOR processCommand, uint16_t port = 23, bool echo = false)
        : _state{0}, _port{port}, _echo{echo}, _processCommand{processCommand},
          _newClient{nullptr}, _clientList{nullptr}, _debugState{nullptr},
          _displayOptions{nullptr}, WiFiServer()
    {
    }

    // Destructor
    ~R4A_TELNET_SERVER()
    {
        if (_newClient)
            delete _newClient;
        while (_clientList)
        {
            R4A_TELNET_CLIENT * client = _clientList;
            _clientList = client->_nextClient;
            client->stop();
            delete client;
        }
    }

    // Display the client list
    void displayClientList(Print * display = &Serial);

    // Get the telnet port number
    //  Returns the telnet port number set during initalization
    uint16_t port();

    // Start and update the telnet server
    // Inputs:
    //   wifiConnected: Set true when WiFi is connected to an access point
    //                  and false otherwise
    void update(bool wifiConnected);
};

#endif  // __R4A_ROBOT_H__
