/**********************************************************************
  Waypoint.cpp

  Waypoint support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Determine the central angle between two points on a sphere
// See https://en.wikipedia.org/wiki/Haversine_formula
// Inputs:
//   point: Address of an R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns the central angle between the two points on the sphere
double r4aCentralAngle(R4A_LAT_LONG_POINT_PAIR * point)
{
    double centralAngle;
    double cosLatitude1;
    double cosLatitude2;
    double cosDeltaLatitude;
    double cosDeltaLongitude;
    double deltaLatitude;
    double deltaLongitude;
    double radicand;
    double squareRoot;
    double thirdTerm;

    // Compute the deltas
    deltaLatitude = point->current.latitude - point->previous.latitude;
    deltaLongitude = point->current.longitude - point->previous.longitude;

    // Compute the cosines
    cosLatitude1 = cos(point->current.latitude);
    cosLatitude2 = cos(point->previous.latitude);
    cosDeltaLatitude = cos(deltaLatitude);
    cosDeltaLongitude = cos(deltaLongitude);

    // Compute the third term in the radicand
    thirdTerm = cosLatitude1 * cosLatitude2 * (1. - cosDeltaLongitude);

    // Compute the radicand, number under the square root sign
    // See https://en.wikipedia.org/wiki/Square_root
    radicand = (1. - cosDeltaLatitude + thirdTerm) / 2.;

    // Compute the square root of the radicand
    squareRoot = sqrt(radicand);

    // Compute the central angle
    centralAngle = 2. * asin(squareRoot);
    return centralAngle;
}

//*********************************************************************
// Compute the heading
// Inputs:
//   heading: Address of a R4A_HEADING object
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
// Determine the ellipsoidal flattening
// See https://en.wikipedia.org/wiki/Flattening
// Inputs:
//   longRadius: The longer of the two radii of the ellipsoid
//   shortRadius: The shorter of the two radii of the ellipsoid
// Outputs:
//   Returns the flattening value
double r4aFlatening(double longRadius, double shortRadius)
{
    return (longRadius - shortRadius) / longRadius;
}

//*********************************************************************
// Determine the haversine distance between two points on a sphere
// See https://en.wikipedia.org/wiki/Haversine_formula
// Inputs:
//   radius: Radius of the sphere
//   point: Address of an R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns the great circle distance between the two points on the sphere
double r4aHaversineDistance(double radius, R4A_LAT_LONG_POINT_PAIR * point)
{
    double distance;

    distance = radius * r4aCentralAngle(point);
    return distance;
}

//*********************************************************************
// Determine the Lambert distance between two points on an ellipsoid
// See https://www.calculator.net/distance-calculator.html
// Inputs:
//   longRadius: Longer radius of the ellipsoid
//   shortRadius: Shorter radius of the ellipsoid
//   point: Address of an R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns the distance between the two points on the ellipsoid.  Note
//   that the units of the large and small radii must match!
double r4aLambertDistance(double longRadius,
                          double shortRadius,
                          R4A_LAT_LONG_POINT_PAIR * point)
{
    double b1;
    double b2;
    double centralAngle;
    double cos2hca;
    double cos2p;
    double cos2q;
    double coshca;
    double cosp;
    double cosq;
    double distance;
    double flatening;
    double p;
    double q;
    double sin2hca;
    double sin2p;
    double sin2q;
    double sinca;
    double x;
    double y;

    // Compute the earth's flatening
    flatening = r4aFlatening(longRadius, shortRadius);

    // Get the central angle
    centralAngle = r4aCentralAngle(point);
    sinca = sin(centralAngle);
    coshca = cos(centralAngle / 2.);

    // Compute the angles
    b1 = atan((1. - flatening) * tan(point->current.latitude));
    b2 = atan((1. - flatening) * tan(point->previous.latitude));

    // Compute p and q
    p = (b1 + b2) / 2;
    cosp = cos(p);

    q = (b2 - b1) / 2;
    cosq = cos(q);

    // Compute the cos^2 and sin^2 values
    cos2p = cosp * cosp;
    sin2p = 1. - cos2p;
    cos2q = cosq * cosq;
    sin2q = 1. - cos2q;
    cos2hca = coshca * coshca;
    sin2hca = 1. - cos2hca;

    // Compute x and y
    x = (centralAngle - sinca) * (sin2p * cos2q / cos2hca);
    y = (centralAngle + sinca) * (cos2p * sin2q / sin2hca);

    // Compute the distance
    distance = longRadius * (centralAngle - (flatening * (x + y) / 2));
    return distance;
}

//*********************************************************************
// Determine the haversine distance between two points on a earth
// See https://en.wikipedia.org/wiki/Haversine_formula
// Inputs:
//   radius: Radius of the sphere
//   point: Address of an R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns the great circle distance between the two points on the earth
//   in kilometers
double r4aWaypointHaversineDistance(R4A_LAT_LONG_POINT_PAIR * point)
{
    double distance;

    // Compute the distance
    distance = R4A_EARTH_EQUATORIAL_RADIUS_KM * r4aCentralAngle(point);
    return distance;
}

//*********************************************************************
// Determine the Lambert distance between two points on a earth
// See https://www.calculator.net/distance-calculator.html
// Inputs:
//   point: Address of an R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns the ellipsoidal distance between the two points on the earth
//   in kilometers
double r4aWaypointLambertDistance(R4A_LAT_LONG_POINT_PAIR * point)
{
    double distance;

    // Compute the distance
    distance = r4aLambertDistance(R4A_EARTH_EQUATORIAL_RADIUS_KM,
                                  R4A_EARTH_POLE_RADIUS_KM,
                                  point);
    return distance;
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
