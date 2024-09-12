/**********************************************************************
  Stricmp.cpp

  Robots-For-All (R4A)
  Compare two strings ignoring case
**********************************************************************/

#include "R4A_Robot.h"

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
