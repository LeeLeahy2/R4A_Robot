/**********************************************************************
  Waypoint.cpp

  Waypoint support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Determine if the waypoint was reached
// Inputs:
//   wpLatitude: Waypoint latitude
//   wpLongitude: Wapoint longitude
//   latitude: Current latitude
//   longitude: Current longitude
// Outputs:
//   Returns true if the position is close enough to the waypoint
bool r4aWaypointReached(double wpLatitude,
                        double wpLongitude,
                        double latitude,
                        double longitude)
{
    double latInches;
    double longInches;

    // Compute the offset in inches
    latInches = abs(latitude - wpLatitude) * R4A_GNSS_EARTH_LAT_RADIUS_IPD;
    longInches = abs(longitude - wpLongitude) * R4A_GNSS_EARTH_LONG_RADIUS_IPD;

    // Determine if the waypoint was reached
    return (((latInches * latInches) + (longInches * longInches))
        <= (R4A_INCHES_PER_FOOT * R4A_INCHES_PER_FOOT));
}
