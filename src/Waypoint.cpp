/**********************************************************************
  Waypoint.cpp

  Waypoint support
**********************************************************************/

#include "R4A_Robot.h"

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
