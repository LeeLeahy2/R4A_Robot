/**********************************************************************
  Telnet_Context.cpp

  Robots-For-All (R4A)
  Telnet context support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Initialize the R4A_TELNET_CONTEXT data structure
// Inputs:
//   context: Address of an R4A_TELNET_CONTEXT data structure
//   menuTable: Address of table containing the menu descriptions, the
//              main menu must be the first entry in the table.
//   menuEntries: Number of entries in the menu table
//   displayOptions: Display the telnet header options
//   echo: Echo the input characters
//   blankLineBeforePreMenu: Display a blank line before the preMenu
//   blankLineBeforeMenuHeader: Display a blank line before the menu header
//   blankLineAfterMenuHeader: Display a blank line after the menu header
//   alignCommands: Align the commands
//   blankLineAfterMenu: Display a blank line after the menu
void r4aTelnetContextBegin(R4A_TELNET_CONTEXT * context,
                           const R4A_MENU_TABLE * menuTable,
                           int menuTableEntries,
                           bool displayOptions,
                           bool echo,
                           bool blankLineBeforePreMenu,
                           bool blankLineBeforeMenuHeader,
                           bool blankLineAfterMenuHeader,
                           bool alignCommands,
                           bool blankLineAfterMenu)
{
    // Initialize the command string
    context->_command = String("");

    // Set the telnet options
    context->_displayOptions = displayOptions;
    context->_echo = echo;

    // Set the menu options
    r4aMenuBegin(&context->_menu,
                 menuTable,
                 menuTableEntries,
                 blankLineBeforePreMenu,
                 blankLineBeforeMenuHeader,
                 blankLineAfterMenuHeader,
                 alignCommands,
                 blankLineAfterMenu);
}

//*********************************************************************
// Finish creating the network client
// Inputs:
//   contextAddr: Buffer to receive the address of an R4A_TELNET_CONTEXT
//                data structure allocated by this routine
//   client: Address of a NetworkClient object
//   menuTable: Address of table containing the menu descriptions, the
//              main menu must be the first entry in the table.
//   menuEntries: Number of entries in the menu table
// Outputs:
//   Returns true if the routine was successful and false upon failure.
bool r4aTelnetContextCreate(void ** contextAddr,
                            NetworkClient * client,
                            const R4A_MENU_TABLE * menuTable,
                            int menuTableEntries,
                            bool displayOptions,
                            bool echo,
                            bool blankLineBeforePreMenu,
                            bool blankLineBeforeMenuHeader,
                            bool blankLineAfterMenuHeader,
                            bool alignCommands,
                            bool blankLineAfterMenu)
{
    R4A_TELNET_CONTEXT * context;

    // Return an optional object address to be used as a parameter for
    // r4aTelnetClientProcessInput
    context = (R4A_TELNET_CONTEXT *)r4aMalloc(sizeof(*context), "Telnet context (context)");
    if (context)
    {
        // Save the allocated context address
        *contextAddr = (void *)context;

        // Initialize the menu
        memset(context, 0, sizeof(*context));
        r4aTelnetContextBegin(context,
                              menuTable,
                              menuTableEntries,
                              displayOptions,
                              echo,
                              blankLineBeforePreMenu,
                              blankLineBeforeMenuHeader,
                              blankLineAfterMenuHeader,
                              alignCommands,
                              blankLineAfterMenu);

        // Display the menu
        r4aMenuProcess(&context->_menu, nullptr, client);
    }

    return (context != nullptr);
}

//*********************************************************************
// Clean up after the parameter object returned by r4aTelnetContextCreate
// Inputs:
//   contextAddr: Buffer to containing the address of an R4A_TELNET_CONTEXT
//                data structure allocated by r4aTelnetContextCreate
void r4aTelnetContextDelete(void ** contextAddr)
{
    if (*contextAddr)
    {
        r4aFree (*contextAddr, "Telnet context (context)");
        *contextAddr = nullptr;
    }
}

//*********************************************************************
// Process input from the telnet client
// Inputs:
//   contextData: Address of an R4A_TELNET_CONTEXT data structure allocated
//                by r4aTelnetContextCreate
//   client: Address of a NetworkClient object
// Outputs:
//   Returns true if the client is done (requests exit)
bool r4aTelnetContextProcessInput(void * contextData,
                                  NetworkClient * client)
{
    bool clientDone;
    const char * command;
    R4A_TELNET_CONTEXT * context;
    String * line;
    uint8_t option;
    uint8_t parameter;

    context = (R4A_TELNET_CONTEXT *)contextData;

    // Strip any telnet header
    clientDone = false;
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
            if (context->_displayOptions)
                Serial.printf("Telnet Client %s:%d ignoring option 0xff 0x%02x 0x%02x\r\n",
                              client->remoteIP().toString().c_str(),
                              client->remotePort(), option, parameter);
        }
    }

    // Get a command from this client
    line = r4aReadLine(context->_echo, &context->_command, client);

    // If a command is available, process it
    if (line)
    {
        // Process the command
        command = line->c_str();
        clientDone = r4aMenuProcess(&context->_menu, command, client);
        if (!clientDone)
            // Display the menu
            r4aMenuProcess(&context->_menu, nullptr, client);

        // Start building the next command
        context->_command = "";
    }

    // Notify the upper layers when the client wants to exit
    return clientDone;
}
