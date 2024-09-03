/**********************************************************************
  WiFi.cpp

  WiFi stream support
**********************************************************************/

#include "R4A_Common.h"

//*********************************************************************
// Read a line of input from a WiFi client into a String
String * r4aReadLine(bool echo, String * buffer, WiFiClient * port)
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

        // Echo the linefeed
        else if (echo && (data == '\n'))
            port->println();

        // Echo a carriage return and linefeed
        else if (data == '\r')
        {
            if (echo)
                port->println();
            line = buffer;
            break;
        }
    }

    // Return the line when it is complete
    return line;
}
