/**********************************************************************
  Robot.cpp

  Robots-For-All (R4A)
  Implement the R4A_ROBOT class
**********************************************************************/

#include "R4A_Robot.h"

#define SWITCH_STATE(newState)  __atomic_exchange_1((uint8_t *)&robot->_state, newState, 0)

//*********************************************************************
// Private support functions
//*********************************************************************

//*********************************************************************
// Run the robot challenge
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   currentMsec: Milliseconds since boot
void r4aRobotRunning(R4A_ROBOT * robot,
                     uint32_t currentMsec)
{
    R4A_ROBOT_CHALLENGE * challenge;

    // Synchronize with the stop routine
    robot->_busy = true;

    // Is the robot challenge still running
    challenge = (R4A_ROBOT_CHALLENGE *)robot->_challenge;
    if (challenge)
    {
        // Determine the challenge should stop
        if (((int32_t)(robot->_endMsec - currentMsec)) > 0)
            // Perform the robot challenge
            challenge->_challenge(challenge);
        else
        {
            log_v("Robot: Challenge duration has expired");

            // Stop the robot
            r4aRobotStop(robot, currentMsec);
        }
    }

    // Release the synchronation with the stop routine
    robot->_busy = false;
}

//*********************************************************************
// Perform the initial delay
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   currentMsec: Milliseconds since boot
void r4aRobotInitialDelay(R4A_ROBOT * robot,
                          uint32_t currentMsec)
{
    R4A_ROBOT_CHALLENGE * challenge;
    int32_t deltaTime;
    uint8_t previousState;

    // Synchronize with the stop routine
    robot->_busy = true;

    // Determine if the initial delay is complete
    challenge = (R4A_ROBOT_CHALLENGE *)robot->_challenge;
    if (challenge)
    {
        deltaTime = currentMsec - robot->_startMsec;
        if (deltaTime >= 0)
        {
            log_v("Robot: Start delay complete");

            // Notify the challenge of the start
            if (challenge->_start)
            {
                log_v("Robot: Calling challenge->_start");
                challenge->_start(challenge);
            }

            // Switch to running the robot
            previousState = SWITCH_STATE(ROBOT_STATE_RUNNING);
            if (previousState == ROBOT_STATE_COUNT_DOWN)
            {
                log_v("Robot: Switched to RUNNING state");
                r4aRobotRunning(robot, currentMsec);
            }
            else
            {
                // Restore the state
                SWITCH_STATE(previousState);
                log_v("Robot: Switched to previous state, %d", previousState);
            }
        }
        else
        {
            // Display the time
            if (currentMsec >= robot->_nextDisplayMsec)
            {
                robot->_nextDisplayMsec += 100;
                if (robot->_displayTime)
                    robot->_displayTime(robot->_startMsec - robot->_nextDisplayMsec);
            }
        }
    }

    // Release the synchronation with the stop routine
    robot->_busy = false;
}

//*********************************************************************
// Wait after stopping the robot before switching to idle
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
void r4aRobotStopped(R4A_ROBOT * robot,
                     uint32_t currentMsec)
{
    // Initialize the delay
    if (robot->_idleMsec == 0)
        robot->_idleMsec = currentMsec;

    // Delay for a while
    if ((currentMsec - robot->_stopMsec) >= robot->_afterRunMsec)
    {
        r4aLEDsOff();
        log_v("Robot: Switching to IDLE state");
        SWITCH_STATE(ROBOT_STATE_IDLE);
    }
}

//*********************************************************************
// Public API functions
//*********************************************************************

//*********************************************************************
// Initialize the robot data structure
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   core: CPU core that is running the robot layer
//   startDelaySec: Number of seconds before the robot starts a challenge
//   afterRunSec: Number of seconds after the robot completes a challenge
//                before the robot layer switches back to the idle state
//   idle: Address of the idle routine, may be nullptr
//   displayTime: Address of the routine to display the time, may be nullptr
void r4aRobotInit(R4A_ROBOT * robot,
                  int core,
                  uint32_t startDelaySec,
                  uint32_t afterRunSec,
                  R4A_ROBOT_TIME_CALLBACK idle,
                  R4A_ROBOT_TIME_CALLBACK displayTime)
{
    robot->_afterRunMsec = afterRunSec * R4A_MILLISECONDS_IN_A_SECOND;
    robot->_busy = false;
    robot->_challenge = nullptr;
    robot->_core = core;
    robot->_displayTime = displayTime;
    robot->_endMsec = 0;
    robot->_idle = idle;
    robot->_initMsec = 0;
    robot->_nextDisplayMsec = 0;
    robot->_startDelayMsec = startDelaySec * R4A_MILLISECONDS_IN_A_SECOND;
    robot->_startMsec = 0;
    robot->_state = ROBOT_STATE_IDLE;
    robot->_stopMsec = 0;
}

//*********************************************************************
// Determine if the robot layer is active
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
// Outputs:
//   Returns true when the challenge is running and false otherwise
bool r4aRobotIsActive(R4A_ROBOT * robot)
{
    return ((robot->_state == ROBOT_STATE_COUNT_DOWN)
         || (robot->_state == ROBOT_STATE_RUNNING));
}

//*********************************************************************
// Determine if it is possible to start the robot
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   challenge: Address of challenge object
//   display: Device used for output
bool r4aRobotStart(R4A_ROBOT * robot,
                   R4A_ROBOT_CHALLENGE * challenge,
                   Print * display)
{
    uint32_t currentMsec;
    uint32_t hours;
    uint32_t minutes;
    R4A_ROBOT_CHALLENGE * previousChallenge;
    uint32_t seconds;

    log_v("Robot: r4aRobotStart called");

    // Only initialize the robot once
    previousChallenge = (R4A_ROBOT_CHALLENGE *)robot->_challenge;
    if (previousChallenge)
    {
        display->printf("ERROR: Robot already running %s!", previousChallenge->_name);
        return false;
    }

    // Prevent initialization when challenge is not specified
    if (!challenge->_challenge)
    {
        display->printf("ERROR: Robot challenge not specified!\r\n");
        return false;
    }

    // Synchronize with the stop routine
    robot->_busy = true;

    // Compute the times for the challenge
    currentMsec = millis();
    robot->_idleMsec = 0;
    robot->_initMsec = currentMsec;
    robot->_nextDisplayMsec = currentMsec;
    robot->_startMsec = robot->_initMsec + robot->_startDelayMsec;
    robot->_endMsec = robot->_startMsec + (challenge->_duration * R4A_MILLISECONDS_IN_A_SECOND);

    // Display the start delay time
    display->printf("Robot: Delaying %ld seconds before starting %s\r\n",
                    robot->_startDelayMsec / 1000, challenge->_name);

    // Split the duration
    seconds = challenge->_duration;
    minutes = seconds / 60;
    seconds -= minutes * 60;
    hours = minutes / 60;
    minutes -= hours * 60;

    // Display the time
    display->printf("Robot: %s challenge duration %ld:%02ld:%02ld\r\n",
                    challenge->_name, hours, minutes, seconds);
    if (robot->_displayTime)
        robot->_displayTime(robot->_startMsec - robot->_nextDisplayMsec);

    // Call the initialization routine
    if (challenge->_init)
    {
        log_v("Robot: Calling challenge->_init");
        challenge->_init(challenge);
    }

    // Start the robot
    robot->_challenge = challenge;    // Update the LED colors

    log_v("Robot: Calling r4aLEDUpdate");
    r4aLEDUpdate(true);

    log_v("Robot: Switching state to COUND_DOWN");
    SWITCH_STATE(ROBOT_STATE_COUNT_DOWN);

    // Release the synchronation with the stop routine
    robot->_busy = false;
    return true;
}

//*********************************************************************
// Stop the robot
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   currentMsec: Milliseconds since boot
//   display: Device used for output
void r4aRobotStop(R4A_ROBOT * robot,
                  uint32_t currentMsec,
                  Print * display)
{
    R4A_ROBOT_CHALLENGE * challenge;
    uint32_t hours;
    uint32_t milliseconds;
    uint32_t minutes;
    uint32_t seconds;
    uint8_t state;

    log_v("Robot: r4aRobotStop called");

    // Stop the robot just once by setting _state to ROBOT_STATE_STOP
    log_v("Robot: Switching state to STOP");
    state = SWITCH_STATE(ROBOT_STATE_STOP);
    if ((state == ROBOT_STATE_RUNNING) || (state == ROBOT_STATE_COUNT_DOWN))
    {
        robot->_stopMsec = currentMsec;

        // Wait for the I2C bus to be free, robot->_core 0 idle
        log_v("Robot: Wait for I2C to be idle");
        if (robot->_core != xPortGetCoreID())
            while (robot->_busy)
            {
            }
        log_v("Robot: I2C is idle");

        // Call the challenge stop routine to stop the motors
        challenge = (R4A_ROBOT_CHALLENGE *)robot->_challenge;
        if (challenge->_stop)
        {
            log_v("Robot: Calling challenge->_stop");
            challenge->_stop(challenge);
        }

        // Display the runtime
        if (display)
        {
            // Split the runtime
            milliseconds = currentMsec - robot->_startMsec;
            seconds = milliseconds / R4A_MILLISECONDS_IN_A_SECOND;
            milliseconds -= seconds * R4A_MILLISECONDS_IN_A_SECOND;
            minutes = seconds / R4A_SECONDS_IN_A_MINUTE;
            seconds -= minutes * R4A_SECONDS_IN_A_MINUTE;
            hours = minutes / R4A_MINUTES_IN_AN_HOUR;
            minutes -= hours * R4A_MINUTES_IN_AN_HOUR;
            display->printf("Robot: Stopped %s, runtime: %ld:%02ld:%02ld.%03ld\r\n",
                            challenge->_name, hours, minutes, seconds, milliseconds);
        }

        // Display the runtime
        if (robot->_displayTime)
        {
            log_v("Robot: Calling robot->_displayTime");
            robot->_displayTime(robot->_stopMsec - robot->_startMsec);
        }

        // Done with this challenge
        robot->_challenge = nullptr;
    }
}

//*********************************************************************
// Update the robot state
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   currentMsec: Milliseconds since boot
void r4aRobotUpdate(R4A_ROBOT * robot,
                    uint32_t currentMsec)
{
    uint8_t state;

    // Process the robot state
    state = robot->_state;
    if (state == ROBOT_STATE_RUNNING)
        r4aRobotRunning(robot, currentMsec);
    else if (state == ROBOT_STATE_COUNT_DOWN)
        r4aRobotInitialDelay(robot, currentMsec);
    else if (state == ROBOT_STATE_STOP)
        r4aRobotStopped(robot, currentMsec);
    else if (state == ROBOT_STATE_IDLE)
    {
        if (robot->_idle)
            robot->_idle(currentMsec);
    }
    else
    {
        Serial.printf("ERROR: Unknown robot state %d\r\n", state);
        r4aReportFatalError("Unknown robot state");
    }
}
