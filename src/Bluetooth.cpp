/**********************************************************************
  Bluetooth.cpp

  Robots-For-All (R4A)
  Bluetooth stream support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Read a line of input from a Serial port into a String
String * r4aReadLine(bool echo, String * buffer, BluetoothSerial * port)
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
// Process Bluetooth menu item
// Returns true when the client exits the menu system and false otherwise
bool r4aBluetoothMenu(R4A_MENU * menu, BluetoothSerial * port)
{
    const char * command;
    bool done;
    String * line;
    static String serialBuffer;

    // Process input from the serial port
    done = false;
    line = r4aReadLine(true, &serialBuffer, port);
    if (line)
    {
        // Get the command
        command = line->c_str();

        // Process the command
        done = r4aMenuProcess(menu, command, port);
        if (!done)
            // Display the menu
            r4aMenuProcess(menu, nullptr, port);

        // Start building the next command
        serialBuffer = "";
    }

    // Return the exit request status
    return done;
}
