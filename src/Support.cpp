/**********************************************************************
  Support.cpp

  Robots-For-All (R4A)
  Common support routines
**********************************************************************/

#include "R4A_Robot.h"         // Robots-For-All robot support

//*********************************************************************
// Get the parameter
// Inputs:
//   parameter: Address of address of a zero terminated string of characters
// Outputs:
//   Returns the address of the next parameter
uint8_t * r4aSupportGetParameter(uint8_t ** parameter)
{
    uint8_t * addr;
    uint8_t data;

    // Remove the white space before the parameter
    addr = r4aSupportRemoveWhiteSpace(*parameter);
    *parameter = addr;

    // Locate the end of the parameter
    while (*addr)
    {
        data = *addr;
        if ((data == ' ') || (data == '/t') || (data == '/r') || (data == '/n'))
        {
            *addr++ = 0;
            break;
        }
        addr++;
    }

    // Return the address of the next parameter
    return addr;
}

//*********************************************************************
// Remove white space before a parameter
// Inputs:
//   parameter: Address of a zero terminated string of characters containing
//              leading white space and a parameter
// Outputs:
//   Returns the address of the parameter
uint8_t * r4aSupportRemoveWhiteSpace(uint8_t * parameter)
{
    // Remove the white space before the parameter
    while (*parameter)
    {
        if ((*parameter != ' ') && (*parameter != '/t'))
            break;
        parameter++;
    }
    return parameter;
}

//*********************************************************************
// Trim white space at the end of the parameter
// Inputs:
//   parameter: Address of a zero terminated string of characters containing
//              leading white space and a parameter
// Outputs:
//   Returns the address of the parameter
void r4aSupportTrimWhiteSpace(uint8_t * parameter)
{
    uint8_t data;
    uint8_t * end;

    // Remove the white space after the parameter
    end = &parameter[strlen((char *)parameter)];
    while (end > parameter)
    {
        data = *--end;
        if ((data != ' ') && (data != '/t') && (data != '\r') && (data != '\n'))
        {
            // Zero terminate the string
            *++end = 0;
            return;
        }
    }
}
