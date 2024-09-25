/**********************************************************************
  NTP.ino

  Robots-For-All (R4A)
  Get the time from the internet.
**********************************************************************/

#include "R4A_Robot.h"

//****************************************
// Constants
//****************************************

enum R4A_NTP_STATE
{
    R4A_NTP_STATE_WAIT_FOR_WIFI = 0,
    R4A_NTP_STATE_GET_WIFI_UDP,
    R4A_NTP_STATE_GET_NTP_CLIENT,
    R4A_NTP_STATE_NTP_CLIENT_BEGIN,
    R4A_NTP_STATE_GET_INITIAL_TIME,
    R4A_NTP_STATE_TIME_UPDATE,
    R4A_NTP_STATE_FREE_NTP_CLIENT,
    R4A_NTP_STATE_FREE_WIFI_UDP
};

const char * const r4aNtpStateName[] =
{
    "R4A_NTP_STATE_WAIT_FOR_WIFI",
    "R4A_NTP_STATE_GET_WIFI_UDP",
    "R4A_NTP_STATE_GET_NTP_CLIENT",
    "R4A_NTP_STATE_NTP_CLIENT_BEGIN",
    "R4A_NTP_STATE_GET_INITIAL_TIME",
    "R4A_NTP_STATE_TIME_UPDATE",
    "R4A_NTP_STATE_FREE_NTP_CLIENT",
    "R4A_NTP_STATE_FREE_WIFI_UDP"
};
const uint8_t r4aNtpStateNameCount = sizeof(r4aNtpStateName) / sizeof(r4aNtpStateName[0]);

//****************************************
// Globals
//****************************************

bool r4aNtpDebugStates; // Set true to display state changes

//****************************************
// Locals
//****************************************

static NTPClient * r4aNtpClient;
static bool r4aNtpDisplayInitialTime;
static uint8_t r4aNtpState;
static long r4aNtpTimeZoneOffsetSeconds;
static WiFiUDP * r4aNtpUDP;

//*********************************************************************
// Display the date and time
void r4aNtpDisplayDateTime(Print * display)
{
    time_t seconds = r4aNtpGetEpochTime();
    display->printf("%s %s\r\n",
                    r4aNtpGetDate(seconds).c_str(),
                    r4aNtpGetTime24(seconds).c_str());
}

//*********************************************************************
// Get the date string
// Returns the date as yyyy-mm-dd or "Time not set"
String r4aNtpGetDate(uint32_t seconds)
{
    char date[128];

    if (!seconds)
        return String("Time not set");

    // Format the date
    sprintf(date, "%4d-%02d-%02d", year(seconds), month(seconds), day(seconds));
    return String(date);
}

//*********************************************************************
// Get the number of seconds from 1 Jan 1970
//   Returns the number of seconds from 1 Jan 1970
uint32_t r4aNtpGetEpochTime()
{
    time_t seconds;

    if ((r4aNtpState < R4A_NTP_STATE_GET_INITIAL_TIME)
        || (r4aNtpState > R4A_NTP_STATE_TIME_UPDATE))
        return 0;
    seconds = r4aNtpClient->getEpochTime();
    return seconds;
}

//*********************************************************************
// Get the time as hh:mm:ss
String r4aNtpGetTime()
{
    if (!r4aNtpIsTimeValid())
        return "Time not set";
    return r4aNtpClient->getFormattedTime();
}

//*********************************************************************
// Get the time string in 12 hour format
// Returns time as hh:mm:ss xM or "Time not set"
String r4aNtpGetTime12(uint32_t seconds)
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
String r4aNtpGetTime24(uint32_t seconds)
{
    char time[128];

    if (!seconds)
        return String("Time not set");

    // Format the date
    sprintf(time, "%2d:%02d:%02d", hour(seconds), minute(seconds), second(seconds));
    return String(time);
}

//*********************************************************************
// Determine if the time is valid
bool r4aNtpIsTimeValid()
{
    return (r4aNtpState == R4A_NTP_STATE_TIME_UPDATE);
}

//*********************************************************************
// Update the NTP state variable
void r4aNtpSetState(uint8_t newState)
{
    const char * stateName;
    const char * newStateName;

    // Get the state names
    if (r4aNtpDebugStates)
    {
        if (newState >= r4aNtpStateNameCount)
            newStateName = "Unknown";
        else
            newStateName = r4aNtpStateName[newState];
        if (r4aNtpState >= r4aNtpStateNameCount)
            stateName = "Unknown";
        else
            stateName = r4aNtpStateName[r4aNtpState];

        // Display the state transition
        Serial.printf("(%d) %s --> %s (%d)\r\n",
                      r4aNtpState, stateName, newStateName, newState);
    }

    // Update the state
    r4aNtpState = newState;
}

//*********************************************************************
// Set the time zone
void r4aNtpSetTimeZone(long timeZoneOffsetSeconds)
{
    r4aNtpTimeZoneOffsetSeconds = timeZoneOffsetSeconds;
    if (r4aNtpIsTimeValid())
        r4aNtpClient->setTimeOffset(timeZoneOffsetSeconds);
}

//*********************************************************************
// Initialize the NTP server
void r4aNtpSetup(long timeZoneOffsetSeconds, bool displayInitialTime)
{
    r4aNtpTimeZoneOffsetSeconds = timeZoneOffsetSeconds;
    r4aNtpDisplayInitialTime = displayInitialTime;
}

//*********************************************************************
// Update the NTP client and system time
void r4aNtpUpdate(bool wifiConnected)
{
    switch (r4aNtpState)
    {
    default:
        break;

    case R4A_NTP_STATE_WAIT_FOR_WIFI:
        // Wait until WiFi is available
        if (wifiConnected)
            r4aNtpSetState(R4A_NTP_STATE_GET_WIFI_UDP);
        break;

    case R4A_NTP_STATE_GET_WIFI_UDP:
        // Determine if the network has failed
        if (!wifiConnected)
            r4aNtpSetState(R4A_NTP_STATE_WAIT_FOR_WIFI);
        else
        {
            // Allocate the WiFi UDP object
            r4aNtpUDP = new WiFiUDP();
            if (r4aNtpUDP)
                r4aNtpSetState(R4A_NTP_STATE_GET_NTP_CLIENT);
        }
        break;

    case R4A_NTP_STATE_GET_NTP_CLIENT:
        // Determine if the network has failed
        if (!wifiConnected)
            r4aNtpSetState(R4A_NTP_STATE_FREE_WIFI_UDP);
        else
        {
            // Allocate the NTP client object
            r4aNtpClient = new NTPClient(*r4aNtpUDP);
            if (r4aNtpClient)
                r4aNtpSetState(R4A_NTP_STATE_NTP_CLIENT_BEGIN);
        }
        break;

    case R4A_NTP_STATE_NTP_CLIENT_BEGIN:
        // Determine if the network has failed
        if (!wifiConnected)
            r4aNtpSetState(R4A_NTP_STATE_FREE_NTP_CLIENT);
        else
        {
            // Start the NTP object
            r4aNtpClient->begin();
            r4aNtpSetState(R4A_NTP_STATE_GET_INITIAL_TIME);
        }
        break;

    case R4A_NTP_STATE_GET_INITIAL_TIME:
        // Determine if the network has failed
        if (!wifiConnected)
            r4aNtpSetState(R4A_NTP_STATE_FREE_NTP_CLIENT);
        else
        {
            // Attempt to get the time
            r4aNtpClient->update();
            if (r4aNtpClient->isTimeSet())
            {
                r4aNtpClient->setTimeOffset(r4aNtpTimeZoneOffsetSeconds);
                if (r4aNtpDisplayInitialTime)
                    r4aNtpDisplayDateTime();
                r4aNtpSetState(R4A_NTP_STATE_TIME_UPDATE);
            }
        }
        break;

    case R4A_NTP_STATE_TIME_UPDATE:
        // Determine if the network has failed
        if (!wifiConnected)
            r4aNtpSetState(R4A_NTP_STATE_FREE_NTP_CLIENT);
        else
            // Update the time each minute
            r4aNtpClient->update();
        break;

    case R4A_NTP_STATE_FREE_NTP_CLIENT:
        // Done with the NTP client
        delete r4aNtpClient;
        r4aNtpSetState(R4A_NTP_STATE_FREE_WIFI_UDP);
        break;

    case R4A_NTP_STATE_FREE_WIFI_UDP:
        // Done with the WiFi UDP object
        delete r4aNtpUDP;
        r4aNtpSetState(R4A_NTP_STATE_WAIT_FOR_WIFI);
        break;
    }
}
