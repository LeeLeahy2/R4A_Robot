/**********************************************************************
  NTP.ino

  Robots-For-All (R4A)
  Get the time from the internet.
**********************************************************************/

#include "R4A_Robot.h"

//****************************************
// Constants
//****************************************

enum NTP_STATE
{
    NTP_STATE_WAIT_FOR_WIFI = 0,
    NTP_STATE_GET_WIFI_UDP,
    NTP_STATE_GET_NTP_CLIENT,
    NTP_STATE_NTP_CLIENT_BEGIN,
    NTP_STATE_GET_INITIAL_TIME,
    NTP_STATE_TIME_UPDATE,
    NTP_STATE_FREE_NTP_CLIENT,
    NTP_STATE_FREE_WIFI_UDP
};

const char * const ntpStateName[] =
{
    "NTP_STATE_WAIT_FOR_WIFI",
    "NTP_STATE_GET_WIFI_UDP",
    "NTP_STATE_GET_NTP_CLIENT",
    "NTP_STATE_NTP_CLIENT_BEGIN",
    "NTP_STATE_GET_INITIAL_TIME",
    "NTP_STATE_TIME_UPDATE",
    "NTP_STATE_FREE_NTP_CLIENT",
    "NTP_STATE_FREE_WIFI_UDP"
};
const uint8_t ntpStateNameCount = sizeof(ntpStateName) / sizeof(ntpStateName[0]);

//****************************************
// Globals
//****************************************

bool ntpDebugStates; // Set true to display state changes

//****************************************
// Locals
//****************************************

static NTPClient * ntpClient;
static bool ntpDisplayInitialTime;
static uint8_t ntpState;
static long ntpTimeZoneOffsetSeconds;
static WiFiUDP * ntpUDP;

//*********************************************************************
// Initialize the NTP server
void ntpSetup(long timeZoneOffsetSeconds, bool displayInitialTime)
{
    ntpTimeZoneOffsetSeconds = timeZoneOffsetSeconds;
    ntpDisplayInitialTime = displayInitialTime;
}

//*********************************************************************
// Determine if the time is valid
bool ntpIsTimeValid()
{
    return (ntpState == NTP_STATE_TIME_UPDATE);
}

//*********************************************************************
// Set the time zone
void ntpSetTimeZone(long timeZoneOffsetSeconds)
{
    ntpTimeZoneOffsetSeconds = timeZoneOffsetSeconds;
    if (ntpIsTimeValid())
        ntpClient->setTimeOffset(timeZoneOffsetSeconds);
}

//*********************************************************************
// Update the NTP state variable
void ntpSetState(uint8_t newState)
{
    const char * stateName;
    const char * newStateName;

    // Get the state names
    if (ntpDebugStates)
    {
        if (newState >= ntpStateNameCount)
            newStateName = "Unknown";
        else
            newStateName = ntpStateName[newState];
        if (ntpState >= ntpStateNameCount)
            stateName = "Unknown";
        else
            stateName = ntpStateName[ntpState];

        // Display the state transition
        Serial.printf("(%d) %s --> %s (%d)\r\n",
                      ntpState, stateName, newStateName, newState);
    }

    // Update the state
    ntpState = newState;
}

//*********************************************************************
// Get the time as hh:mm:ss
String ntpGetTime()
{
    if (!ntpIsTimeValid())
        return "Time not set";
    return ntpClient->getFormattedTime();
}

//*********************************************************************
// Get the time string in 12 hour format
// Returns time as hh:mm:ss xM or "Time not set"
String ntpGetTime12(uint32_t seconds)
{
    char time[128];

    if (!seconds)
        return String("Time not set");

    // Format the date
    sprintf(time, "%2d:%02d:%02d %s",
            hourFormat12(seconds),
            minute(seconds),
            second(seconds),
            isAM(seconds) ? "AM" : "PM");
    return String(time);
}

//*********************************************************************
// Get the time string in 24 hour format
String ntpGetTime24(uint32_t seconds)
{
    char time[128];

    if (!seconds)
        return String("Time not set");

    // Format the date
    sprintf(time, "%2d:%02d:%02d", hour(seconds), minute(seconds), second(seconds));
    return String(time);
}

//*********************************************************************
// Get the number of seconds from 1 Jan 1970
//   Returns the number of seconds from 1 Jan 1970
uint32_t ntpGetEpochTime()
{
    time_t seconds;

    if ((ntpState < NTP_STATE_GET_INITIAL_TIME)
        || (ntpState > NTP_STATE_TIME_UPDATE))
        return 0;
    seconds = ntpClient->getEpochTime();
    return seconds;
}

//*********************************************************************
// Get the date string
// Returns the date as yyyy-mm-dd or "Time not set"
String ntpGetDate(uint32_t seconds)
{
    char date[128];

    if (!seconds)
        return String("Time not set");

    // Format the date
    sprintf(date, "%4d-%02d-%02d", year(seconds), month(seconds), day(seconds));
    return String(date);
}

//*********************************************************************
// Update the NTP client and system time
void ntpUpdate(bool wifiConnected)
{
    switch (ntpState)
    {
    default:
        break;

    case NTP_STATE_WAIT_FOR_WIFI:
        // Wait until WiFi is available
        if (wifiConnected)
            ntpSetState(NTP_STATE_GET_WIFI_UDP);
        break;

    case NTP_STATE_GET_WIFI_UDP:
        // Determine if the network has failed
        if (!wifiConnected)
            ntpSetState(NTP_STATE_WAIT_FOR_WIFI);
        else
        {
            // Allocate the WiFi UDP object
            ntpUDP = new WiFiUDP();
            if (ntpUDP)
                ntpSetState(NTP_STATE_GET_NTP_CLIENT);
        }
        break;

    case NTP_STATE_GET_NTP_CLIENT:
        // Determine if the network has failed
        if (!wifiConnected)
            ntpSetState(NTP_STATE_FREE_WIFI_UDP);
        else
        {
            // Allocate the NTP client object
            ntpClient = new NTPClient(*ntpUDP);
            if (ntpClient)
                ntpSetState(NTP_STATE_NTP_CLIENT_BEGIN);
        }
        break;

    case NTP_STATE_NTP_CLIENT_BEGIN:
        // Determine if the network has failed
        if (!wifiConnected)
            ntpSetState(NTP_STATE_FREE_NTP_CLIENT);
        else
        {
            // Start the NTP object
            ntpClient->begin();
            ntpSetState(NTP_STATE_GET_INITIAL_TIME);
        }
        break;

    case NTP_STATE_GET_INITIAL_TIME:
        // Determine if the network has failed
        if (!wifiConnected)
            ntpSetState(NTP_STATE_FREE_NTP_CLIENT);
        else
        {
            // Attempt to get the time
            ntpClient->update();
            if (ntpClient->isTimeSet())
            {
                ntpClient->setTimeOffset(ntpTimeZoneOffsetSeconds);
                if (ntpDisplayInitialTime)
                {
                    time_t seconds = ntpGetEpochTime();
                    Serial.printf("%s %s\r\n",
                                  ntpGetDate(seconds).c_str(),
                                  ntpGetTime24(seconds).c_str());
                }
                ntpSetState(NTP_STATE_TIME_UPDATE);
            }
        }
        break;

    case NTP_STATE_TIME_UPDATE:
        // Determine if the network has failed
        if (!wifiConnected)
            ntpSetState(NTP_STATE_FREE_NTP_CLIENT);
        else
            // Update the time each minute
            ntpClient->update();
        break;

    case NTP_STATE_FREE_NTP_CLIENT:
        // Done with the NTP client
        delete ntpClient;
        ntpSetState(NTP_STATE_FREE_WIFI_UDP);
        break;

    case NTP_STATE_FREE_WIFI_UDP:
        // Done with the WiFi UDP object
        delete ntpUDP;
        ntpSetState(NTP_STATE_WAIT_FOR_WIFI);
        break;
    }
}
