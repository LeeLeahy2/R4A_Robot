/**********************************************************************
  Strincmp.cpp

  Robots-For-All (R4A)
  Compare two strings limited to N characters ignoring case
**********************************************************************/

#include "R4A_Robot.h"

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
