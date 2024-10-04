/**********************************************************************
  R4A_Robot.h

  Robots-For-All (R4A)
  Declare the robot support
**********************************************************************/

#ifndef __R4A_ROBOT_H__
#define __R4A_ROBOT_H__

#include "R4A_Common.h"         // Robots-For-All common support

#include <WiFiServer.h>         // Built-in

//****************************************
// Forward Declarations
//****************************************

class R4A_TELNET_SERVER;

//****************************************
// Command Processor API
//****************************************

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

//****************************************
// Menu API
//****************************************

#define R4A_MENU_NONE           0
#define R4A_MENU_MAIN           1

// Forward declarations
typedef struct _R4A_MENU_ENTRY R4A_MENU_ENTRY;

typedef void (*R4A_PRE_MENU_DISPLAY)(Print * display);

typedef struct _R4A_MENU_TABLE
{
    const char * menuName;          // Name of the menu
    R4A_PRE_MENU_DISPLAY preMenu;   // Routine to display data before the menu
    const R4A_MENU_ENTRY * firstEntry; // First entry in the menu
    int32_t menuEntryCount;         // Number of entries in the table
} R4A_MENU_TABLE;

// Process a menu item
// Inputs:
//   menuEntry: Address of the menu entry associated with the command
//   command: Full command line
//   display: Address of the Print object for output
typedef void (*R4A_MENU_ROUTINE)(const struct _R4A_MENU_ENTRY * menuEntry,
                                 const char * command,
                                 Print * display);

// Display help for a menu item
// Inputs:
//   menuEntry: Address of the menu entry to display help
//   align: Zero terminated string of spaces to align the help display
//   display: Address of the Print object for output
typedef void (*R4A_HELP_ROUTINE)(const struct _R4A_MENU_ENTRY * menuEntry,
                                 const char * align,
                                 Print * display);

typedef struct _R4A_MENU_ENTRY
{
    const char * command;           // Command: Expected serial input
    R4A_MENU_ROUTINE menuRoutine;   // Routine to process the menu
    intptr_t menuParameter;         // Parameter for the menu routine
    R4A_HELP_ROUTINE helpRoutine;   // Routine to display the help message
    int align;                      // Command length adjustment for alignment
    const char * helpText;          // Help text to display
} R4A_MENU_ENTRY;

class R4A_MENU
{
  private:

    const R4A_MENU_TABLE * _menu;            // Current menu to display and use
    const R4A_MENU_TABLE * const _menuTable; // Address of all menu descriptions
    const int _menuTableEntries;             // Number of entries in the menu table

  public:

    bool _blankLineBeforePreMenu;       // Display a blank line before the preMenu
    bool _blankLineBeforeMenuHeader;    // Display a blank line before the menu header
    bool _blankLineAfterMenuHeader;     // Display a blank line after the menu header
    bool _alignCommands;                // Align the commands
    bool _blankLineAfterMenu;           // Display a blank line after the menu

    // Constructor
    // Inputs:
    //   menuTable: Address of table containing the menu descriptions, the
    //              main menu must be the first entry in the table.
    //   menuEntries: Number of entries in the menu table
    //   blankLineBeforePreMenu: Display a blank line before the preMenu
    //   blankLineBeforeMenuHeader: Display a blank line before the menu header
    //   blankLineAfterMenuHeader: Display a blank line after the menu header
    //   alignCommands: Align the commands
    //   blankLineAfterMenu: Display a blank line after the menu
    R4A_MENU(const R4A_MENU_TABLE * menuTable,
            int menuEntries,
            bool blankLineBeforePreMenu = true,
            bool blankLineBeforeMenuHeader = true,
            bool blankLineAfterMenuHeader = false,
            bool alignCommands = true,
            bool blankLineAfterMenu = false)
        : _menuTable{menuTable}, _menuTableEntries{menuEntries},
          _blankLineBeforePreMenu{blankLineBeforePreMenu},
          _blankLineBeforeMenuHeader{blankLineBeforeMenuHeader},
          _blankLineAfterMenuHeader{blankLineAfterMenuHeader},
          _alignCommands{alignCommands}, _blankLineAfterMenu{blankLineAfterMenu}
    {
    }

    // Display the menu object contents
    // Inputs:
    //   display: Address of the Print object for output
    void display(Print * display);

    // Process a menu command when specified or display the menu when command
    // is nullptr.
    // Inputs:
    //   menuTable: Address of the address of the menuTable (description of the menu)
    //   command: Command string
    //   display: Address of the Print object for output
    // Outputs:
    //   True when exiting the menu system, false if still in the menu system
    bool process(const char * command,
                 Print * display = &Serial);
};

// Display the boolean as enabled or disabled
// Inputs:
//   menuEntry: Address of the menu entry to display
//   align: Zero terminated string of spaces to align the help display
//   display: Address of the Print object for output
void r4aMenuBoolHelp(const R4A_MENU_ENTRY * menuEntry,
                     const char * align,
                     Print * display);

// Toggle boolean value
// Inputs:
//   menuEntry: Address of the menu entry containing the address to toggle
//   display: Address of the Print object for output
void r4aMenuBoolToggle(const R4A_MENU_ENTRY * menuEntry,
                       const char * command,
                       Print * display);

//****************************************
// Serial API
//****************************************

// Repeatedly display a fatal error message
// Inputs:
//   errorMessage: Zero terminated string of characters containing the
//                 error mesage to be displayed
//   display: Device used for output
void r4aReportFatalError(const char * errorMessage,
                         Print * display = &Serial);

// Process serial menu item
// Inputs:
//   menu: Address of the menu object
void r4aSerialMenu(R4A_MENU * menu);

//****************************************
// Telnet Client API
//****************************************

// Forward declaration
class R4A_TELNET_SERVER;

//*********************************************************************
// Telnet client
class R4A_TELNET_CLIENT : public WiFiClient
{
private:
    R4A_TELNET_CLIENT * _nextClient; // Next client in the server's client list
    String _command; // User command received via telnet
    R4A_MENU _menu;  // Strcture describing the menu system

    // Allow the server to access the command string
    friend R4A_TELNET_SERVER;

public:

    // Constructor
    // Inputs:
    //   menuTable: Address of table containing the menu descriptions, the
    //              main menu must be the first entry in the table.
    //   menuEntries: Number of entries in the menu table
    //   blankLineBeforePreMenu: Display a blank line before the preMenu
    //   blankLineBeforeMenuHeader: Display a blank line before the menu header
    //   blankLineAfterMenuHeader: Display a blank line after the menu header
    //   alignCommands: Align the commands
    //   blankLineAfterMenu: Display a blank line after the menu
    R4A_TELNET_CLIENT(const R4A_MENU_TABLE * menuTable,
                      int menuTableEntries,
                      bool blankLineBeforePreMenu = true,
                      bool blankLineBeforeMenuHeader = true,
                      bool blankLineAfterMenuHeader = false,
                      bool alignCommands = true,
                      bool blankLineAfterMenu = false)
        : _menu{R4A_MENU(menuTable, menuTableEntries, blankLineBeforePreMenu,
                blankLineBeforeMenuHeader, blankLineAfterMenuHeader,
                alignCommands, blankLineAfterMenu)}
    {
    }

    // Allow the R4A_TELNET_SERVER access to the private members
    friend class R4A_TELNET_SERVER;
};

//****************************************
// Telnet Menu API
//****************************************

extern const R4A_MENU_ENTRY r4aTelnetMenuTable[]; // Telnet menu table
#define R4A_TELNET_MENU_ENTRIES       4           // Telnet menu table entries

// Display the telnet clients
// Inputs:
//   command: Zero terminated command string
//   display: Device used for output
void r4aTelnetMenuClients(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

// Display the telnet options
// Inputs:
//   command: Zero terminated command string
//   display: Device used for output
void r4aTelnetMenuOptions(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

// Display the telnet options
// Inputs:
//   command: Zero terminated command string
//   display: Device used for output
void r4aTelnetMenuState(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

// Display the telnet state
// Inputs:
//   display: Device used for output
void r4aTelnetMenuStateDisplay(Print * display);

//****************************************
// Telnet Server API
//****************************************

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
    R4A_TELNET_CLIENT * _newClient; // New client object, ready for listen call
    R4A_TELNET_CLIENT * _clientList; // Singlely linked list of telnet clients
    const R4A_MENU_TABLE * const _menuTable; // Address of all menu descriptions
    const int _menuTableEntries;             // Number of entries in the menu table
    const bool _blankLineBeforePreMenu;      // Display a blank line before the preMenu
    const bool _blankLineBeforeMenuHeader;   // Display a blank line before the menu header
    const bool _blankLineAfterMenuHeader;    // Display a blank line after the menu header
    const bool _alignCommands;               // Align the commands
    const bool _blankLineAfterMenu;          // Display a blank line after the menu

public:

    Print * _debugState; // Address of Print object to display telnet server state changes
    Print * _displayOptions; // Address of Print object to display telnet options

    // Constructor
    // Inputs:
    //   menuTable: Address of table containing the menu descriptions, the
    //              main menu must be the first entry in the table.
    //   menuEntries: Number of entries in the menu table
    //   blankLineBeforePreMenu: Display a blank line before the preMenu
    //   blankLineBeforeMenuHeader: Display a blank line before the menu header
    //   blankLineAfterMenuHeader: Display a blank line after the menu header
    //   alignCommands: Align the commands
    //   blankLineAfterMenu: Display a blank line after the menu
    //   port: Port number for internet connection
    //   echo: When true causes the input characters to be echoed
    R4A_TELNET_SERVER(const R4A_MENU_TABLE * menuTable,
                      int menuTableEntries,
                      bool blankLineBeforePreMenu = true,
                      bool blankLineBeforeMenuHeader = true,
                      bool blankLineAfterMenuHeader = false,
                      bool alignCommands = true,
                      bool blankLineAfterMenu = false,
                      uint16_t port = 23,
                      bool echo = false)
        : _menuTable{menuTable}, _menuTableEntries{menuTableEntries},
          _state{0}, _port{port}, _echo{echo}, _newClient{nullptr},
          _clientList{nullptr}, _debugState{nullptr}, _displayOptions{nullptr},
          _blankLineBeforePreMenu{blankLineBeforePreMenu},
          _blankLineBeforeMenuHeader{blankLineBeforeMenuHeader},
          _blankLineAfterMenuHeader{blankLineAfterMenuHeader},
          _alignCommands{alignCommands}, _blankLineAfterMenu{blankLineAfterMenu},
          WiFiServer()
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

extern class R4A_TELNET_SERVER telnet;  // Server providing telnet access

#endif  // __R4A_ROBOT_H__
