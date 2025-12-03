/**********************************************************************
  Time.cpp

  Robots-For-All (R4A)
  Menu support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Compute the average of a list of microsecond entries
R4A_TIME_USEC_t r4aTimeComputeAverageUsec(R4A_TIME_USEC_t * list,
                                          uint32_t entries,
                                          R4A_TIME_USEC_t * maximumUsec,
                                          R4A_TIME_USEC_t * minimumUsec)
{
    uint64_t averageUsec;
    uint32_t index;
    R4A_TIME_USEC_t maxUsec;
    R4A_TIME_USEC_t minUsec;
    R4A_TIME_USEC_t valueUsec;

    // Compute the average loop times
    averageUsec = 0;
    maxUsec = 0;
    minUsec = (R4A_TIME_USEC_t)-1LL;
    for (index = 0; index < entries; index++)
    {
        valueUsec = list[index];
        averageUsec += valueUsec;
        if (minUsec > valueUsec)
            minUsec = valueUsec;
        if (maxUsec < valueUsec)
            maxUsec = valueUsec;
    }

    // Return the maximum and minimum
    if (maximumUsec)
        *maximumUsec = maxUsec;
    if (minimumUsec)
        *minimumUsec = minUsec;

    // Return the average
    averageUsec /= entries;
    return (R4A_TIME_USEC_t)averageUsec;
}

//*********************************************************************
// Compute the standard deviation of a list of microsecond entries
R4A_TIME_USEC_t r4aTimeComputeStdDevUsec(R4A_TIME_USEC_t * list,
                                         uint32_t entries,
                                         R4A_TIME_USEC_t averageUsec)
{
    R4A_TIME_USEC_t deltaUsec;
    uint32_t index;
    R4A_TIME_USEC_t stdDevUsec;
    uint64_t stdDevUsecSquaredSum;

    // Compute the standard deviation
    stdDevUsecSquaredSum = 0;
    for (index = 0; index < entries; index++)
    {
        deltaUsec = list[index] - averageUsec;
        stdDevUsecSquaredSum += deltaUsec * deltaUsec;
    }
    stdDevUsecSquaredSum /= entries;
    stdDevUsec = (R4A_TIME_USEC_t)sqrt(stdDevUsecSquaredSum);
    return stdDevUsec;
}

//*********************************************************************
// Format the time
void r4aTimeFormatLoopTime(char * buffer, R4A_TIME_USEC_t uSec)
{
    R4A_TIME_USEC_t seconds;

    seconds = uSec / 1000000;
    uSec -= seconds * 1000000;
    if (sizeof(R4A_TIME_USEC_t) == 4)
        sprintf(buffer, "%ld.%06ld", seconds, uSec);
    else
        sprintf(buffer, "%ld.%06ld", seconds, uSec);
}

//*********************************************************************
// Display loop times
void r4aTimeDisplayLoopTimes(Print * display,
                             R4A_TIME_USEC_t * list,
                             uint32_t entries,
                             const char * text)
{
    R4A_TIME_USEC_t averageUsec;
    R4A_TIME_USEC_t maximumUsec;
    R4A_TIME_USEC_t minimumUsec;
    R4A_TIME_USEC_t stdDevUsec;
    char timeAve[32];
    char timeMax[32];
    char timeMin[32];
    char timeStdDev[32];

    // Check for no entries
    if (entries == 0)
    {
        display->printf("                                                    %6ld  %s\r\n",
                        entries, text);
        return;
    }

    // Get the loop times
    averageUsec = r4aTimeComputeAverageUsec(list, entries, &maximumUsec, &minimumUsec);
    stdDevUsec = r4aTimeComputeStdDevUsec(list, entries, averageUsec);

    // Format the loop times
    r4aTimeFormatLoopTime(timeAve, averageUsec);
    r4aTimeFormatLoopTime(timeMax, maximumUsec);
    r4aTimeFormatLoopTime(timeMin, minimumUsec);
    r4aTimeFormatLoopTime(timeStdDev, stdDevUsec);

    // Display the loop times
    display->printf("%11s  %11s  %11s  %11s  %6ld  %s\r\n",
                    timeAve, timeMax, timeMin, timeStdDev, entries, text);
}
