/**********************************************************************
  04_NTP.ino

  Robots-For-All (R4A)
  Example NTP client
**********************************************************************/

#include <R4A_Robot.h>

#include "secrets.h"

#define TIME_ZONE_SECONDS       (-10 * 60 * 60) // Honolulu

//*********************************************************************
// Entry point for the application
void setup()
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println();

    // Start the WiFi network
    WiFi.begin(wifiSSID, wifiPassword);

    // Initialize the NTP client, time
    r4aNtpSetup(TIME_ZONE_SECONDS, true);
}

//*********************************************************************
// Idle loop the application
void loop()
{
    bool wifiConnected;

    // Notify the NTP client of WiFi changes
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    r4aNtpUpdate(wifiConnected);
}
