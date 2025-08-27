/**********************************************************************
  Runtime_Library.cpp

  Robots-For-All (R4A)
  Runtime library support routines
**********************************************************************/

#include "R4A_Robot.h"         // Robots-For-All robot support

//*********************************************************************
// Swap the two bytes
uint16_t r4aBswap16(uint16_t value)
{
    value = ((value >> 8) & 0xff) | ((value & 0xff) << 8);
    return value;
}

//*********************************************************************
// Swap the four bytes
uint32_t r4aBswap32(uint32_t value)
{
    value = ((value >> 24) & 0xff)
          | ((value >> 8) & 0xff00)
          | ((value & 0xff00) << 8)
          | ((value & 0xff) << 24);
    return value;
}

//*********************************************************************
// Compare two strings ignoring case
int r4aStricmp(const char *str1, const char *str2)
{
    int difference;
    char str1Data;

    difference = 0;
    do
    {
        // Compare a character from each string
        str1Data = *str1++;
        difference = tolower(str1Data) - tolower(*str2++);

        // Done when there is a difference found
        if (difference)
            break;

        // Continue scanning remaining characters until the end-of-string
    } while (str1Data);

    // Return the differerence (*str1 - *str2)
    return difference;
}

//*********************************************************************
// Compare two strings limited to N characters ignoring case
int r4aStrincmp(const char *str1, const char *str2, int length)
{
    int difference;
    const char * end;
    char str1Data;

    difference = 0;
    if (length)
    {
        end = &str1[length];
        do
        {
            // Compare a character from each string
            str1Data = *str1++;
            difference = tolower(str1Data) - tolower(*str2++);

            // Done when there is a difference found
            if (difference)
                break;

            // Continue scanning remaining characters until the end-of-string
        } while (str1Data && (end > str1));
    }

    // Return the differerence (*str1 - *str2)
    return difference;
}
