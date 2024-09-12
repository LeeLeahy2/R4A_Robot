/**********************************************************************
  Serial.cpp

  Robots-For-All (R4A)
  Serial stream support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Read a line of input from a Serial port into a String
String * r4aReadLine(bool echo, String * buffer, HardwareSerial * port)
{
    char data;
    String * line;

    // Wait for an input character
    line = nullptr;
    while (port->available())
    {
        // Get the input character
        data = port->read();
        if ((data != '\r') && (data != '\n'))
        {
            // Handle backspace
            if (data == 8)
            {
                // Output a bell when the buffer is empty
                if (buffer->length() <= 0)
                    port->write(7);
                else
                {
                    // Remove the character from the line
                    port->write(data);
                    port->write(' ');
                    port->write(data);

                    // Remove the character from the buffer
                    *buffer = buffer->substring(0, buffer->length() - 1);
                }
            }
            else
            {
                // Echo the character
                if (echo)
                    port->write(data);

                // Add the character to the line
                *buffer += data;
            }
        }

        // Echo a carriage return and linefeed
        else if (data == '\r')
        {
            if (echo)
                port->println();
            line = buffer;
            break;
        }

        // Echo the linefeed
        else if (echo && (data == '\n'))
            port->println();
    }

    // Return the line when it is complete
    return line;
}

//*********************************************************************
// Process serial menu item
void r4aSerialMenu(R4A_COMMAND_PROCESSOR mainMenu)
{
    const char * command;
    String * line;
    static String serialBuffer;
    static R4A_COMMAND_PROCESSOR serialMenu;

    // Process input from the serial port
    line = r4aReadLine(true, &serialBuffer, &Serial);
    if (line)
    {
        // Start from the main menu
        if (!serialMenu)
            serialMenu = mainMenu;

        // Check for a command
        command = line->c_str();
        if (strlen(command))
        {
            // Process the command
            r4aProcessCommand = serialMenu;
            serialMenu(command, &Serial);
            serialBuffer = "";

            // Support sub-menus
            serialMenu = r4aProcessCommand;
        }

        // Display the menu
        if (serialMenu)
            serialMenu(nullptr, &Serial);
    }
}
