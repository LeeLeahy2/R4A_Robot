/**********************************************************************
  Waypoint.cpp

  Waypoint support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Compute the heading
// Inputs:
//   heading: Address of R4A_HEADING object
void r4aComputeHeading(R4A_HEADING * heading)
{
    //                    North
    //                      ^
    //                      | +Lat
    //                      |
    //                      |
    //         -Long        |        +Long
    //   West <-------------+-------------> East
    //                      |
    //                      |
    //                      |
    //                      | -Lat
    //                      v
    //                    South
    //
    // Determine our current heading
    heading->delta.latitude = heading->location->current.latitude - heading->location->previous.latitude;
    heading->delta.longitude = heading->location->current.longitude - heading->location->previous.longitude;
    if (heading->delta.longitude > 180.)
        heading->delta.longitude -= 360.;
    else if (heading->delta.longitude < -180.)
        heading->delta.longitude += 360.;

    // Determine the east-west distance
    heading->eastWestInchesTotal = heading->delta.longitude * 2. * M_PI * R4A_GNSS_EARTH_LONG_RADIUS_IPD;
    heading->eastWestInches = heading->eastWestInchesTotal;
    heading->eastWest = 'E';
    if (heading->eastWestInches < 0)
    {
        heading->eastWest = 'W';
        heading->eastWestInches = -heading->eastWestInches;
    }
    heading->eastWestFeet = heading->eastWestInches / R4A_INCHES_PER_FOOT;
    heading->eastWestInches -= heading->eastWestFeet * R4A_INCHES_PER_FOOT;

    // Determine the north-south distance
    heading->northSouthInchesTotal = heading->delta.latitude * 2. * M_PI * R4A_GNSS_EARTH_LAT_RADIUS_IPD;
    heading->northSouthInches = heading->northSouthInchesTotal;
    heading->northSouth = 'N';
    if (heading->northSouthInches < 0)
    {
        heading->northSouth = 'S';
        heading->northSouthInches = -heading->northSouthInches;
    }
    heading->northSouthFeet = heading->northSouthInches / R4A_INCHES_PER_FOOT;
    heading->northSouthInches -= heading->northSouthFeet * R4A_INCHES_PER_FOOT;

    // Compute the total distance
    heading->inchesTotal = sqrt((heading->eastWestInchesTotal * heading->eastWestInchesTotal)
                                + (heading->northSouthInchesTotal * heading->northSouthInchesTotal));
    heading->inches = heading->inchesTotal;
    heading->feet = heading->inches / R4A_INCHES_PER_FOOT;
    heading->inches -= heading->feet * R4A_INCHES_PER_FOOT;

    // Compute the heading angle
    heading->radians = atan2(heading->eastWestInches, heading->northSouthInches);
    heading->degrees = 180 * heading->radians / M_PI;
    if (heading->degrees > 180.)
        heading->degrees -= 360.;
    else if (heading->degrees < -180.)
        heading->degrees += 360.;
}

//*********************************************************************
// Display the heading
// Inputs:
//   heading: Address of a R4A_HEADING object
//   text: Zero terminated line of text to display
void r4aDisplayHeading(R4A_HEADING * heading, const char * text, Print * display)
{
    // Display the current location, previous location and current heading
    //              -123.123456789   -123.123456789   12345.123   123
    display->printf("\r\n");
    display->printf("                Latitude        Longitude  HPA Meters   SIV\r\n");
    display->printf("          --------------  ---------------  ----------   ---\r\n");
    display->printf("Current   %14.9f   %14.9f   %9.3f   %3d\r\n",
                   heading->location->current.latitude, heading->location->current.longitude,
                   heading->location->current.hpa, heading->location->current.siv);
    display->printf("Previous  %14.9f   %14.9f   %9.3f   %3d\r\n",
                   heading->location->previous.latitude, heading->location->previous.longitude,
                   heading->location->previous.hpa, heading->location->previous.siv);
    display->printf("Delta     %14.9f   %14.9f   %s\r\n",
                   heading->delta.latitude, heading->delta.longitude, text);

    // Display the distance travelled
    display->printf("%c:%4d'%6.3lf\"   %c:%4d'%6.3lf\"   D:%4d'%6.3lf\"   A:%8.3f°\r\n",
                   heading->northSouth, heading->northSouthFeet, heading->northSouthInches,
                   heading->eastWest, heading->eastWestFeet, heading->eastWestInches,
                   heading->feet, heading->inches,
                   heading->degrees);
    display->printf("\r\n");
}

//*********************************************************************
// Determine if the waypoint was reached
// Inputs:
//   point: Address of R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns true if the position is close enough to the waypoint
bool r4aWaypointReached(R4A_LAT_LONG_POINT_PAIR * point)
{
    double latInches;
    double longInches;

    // Compute the offset in inches
    latInches = abs(point->current.latitude - point->previous.latitude)
              * 2. * M_PI * R4A_GNSS_EARTH_LAT_RADIUS_IPD;
    longInches = abs(point->current.longitude - point->previous.longitude)
               * 2. * M_PI * R4A_GNSS_EARTH_LONG_RADIUS_IPD;

    // Determine if the waypoint was reached
    return (((latInches * latInches) + (longInches * longInches))
        <= (R4A_INCHES_PER_FOOT * R4A_INCHES_PER_FOOT));
}
