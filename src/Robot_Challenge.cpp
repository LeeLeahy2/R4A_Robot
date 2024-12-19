/**********************************************************************
  Robot_Challenge.cpp

  Robots-For-All (R4A)
  Implement the R4A_ROBOT_CHALLENGE class
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Constructor
R4A_ROBOT_CHALLENGE::R4A_ROBOT_CHALLENGE(const char * name,
                                         uint32_t durationSec)
    : _name{name}, _duration{durationSec}
{
}

//*********************************************************************
// Get the challenge duration
// Returns the challenge duration in seconds
uint32_t R4A_ROBOT_CHALLENGE::duration()
{
    return _duration;
}

//*********************************************************************
// Get the challenge name
// Returns a zero terminated challenge name string
const char * R4A_ROBOT_CHALLENGE::name()
{
    return _name;
}
