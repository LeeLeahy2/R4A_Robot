/**********************************************************************
  Telnet_Context.cpp

  Robots-For-All (R4A)
  Telnet context support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Finish creating the network client
// Inputs:
//   client: Address of a NetworkClient object
//   menuTable: Address of table containing the menu descriptions, the
//              main menu must be the first entry in the table.
//   menuEntries: Number of entries in the menu table
//   contextData: Buffer to receive the address of an object allocated by
//                this routine
// Outputs:
//   Returns true if the routine was successful and false upon failure.
bool r4aTelnetContextCreate(NetworkClient * client,
                            const R4A_MENU_TABLE * menuTable,
                            int menuTableEntries,
                            void ** contextData)
{
    R4A_TELNET_CONTEXT * context;

    // Return an optional object address to be used as a parameter for
    // r4aTelnetClientProcessInput
    context = new R4A_TELNET_CONTEXT(menuTable, menuTableEntries);
    *contextData = (void *)context;
    if (contextData)
    {
        // Display the menu
        context->_menu.process(nullptr, client);
    }

    return (context != nullptr);
}

//*********************************************************************
// Clean up after the parameter object returned by r4aTelnetClientBegin
// Inputs:
//   contextData: Address of object allocated by r4aTelnetClientBegin
void r4aTelnetContextDelete(void * contextData)
{
    delete (R4A_TELNET_CONTEXT *)contextData;
}

//*********************************************************************
// Process input from the telnet client
// Inputs:
//   client: Address of a NetworkClient object
//   contextData: Address of object allocated by r4aTelnetClientBegin
// Outputs:
//   Returns true if the client is done (requests exit)
bool r4aTelnetContextProcessInput(NetworkClient * client, void * contextData)
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
        clientDone = context->_menu.process(command, client);
        if (!clientDone)
            // Display the menu
            context->_menu.process(nullptr, client);

        // Start building the next command
        context->_command = "";
    }

    // Notify the upper layers when the client wants to exit
    return clientDone;
}
