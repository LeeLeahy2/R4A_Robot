/**********************************************************************
  Robot.cpp

  Robots-For-All (R4A)
  Implement the R4A_ROBOT class
**********************************************************************/

#include "R4A_Robot.h"

#define SWITCH_STATE(newState)      __atomic_exchange_1((uint8_t *)&_state, newState, 0)

//*********************************************************************
// Perform the initial delay
void R4A_ROBOT::initialDelay(uint32_t currentMsec)
{
    R4A_ROBOT_CHALLENGE * challenge;
    int32_t deltaTime;

    // Synchronize with the stop routine
    _busy = true;

    // Determine if the initial delay is complete
    challenge = (R4A_ROBOT_CHALLENGE *)_challenge;
    if (challenge)
    {
        deltaTime = currentMsec - _startMsec;
        if (deltaTime >= 0)
        {
            // Notify the challenge of the start
            challenge->start();

            // Switch to running the robot
            if (SWITCH_STATE(STATE_RUNNING))
                running(currentMsec);
        }
        else
        {
            // Display the time
            if (currentMsec >= _nextDisplayMsec)
            {
                _nextDisplayMsec += 100;
                if (_displayTime)
                    _displayTime(_startMsec - _nextDisplayMsec);
            }
        }
    }

    // Release the synchronation with the stop routine
    _busy = false;
}

//*********************************************************************
// Determine if it is possible to start the robot
bool R4A_ROBOT::init(R4A_ROBOT_CHALLENGE * challenge,
                     uint32_t duration,
                     Print * display)
{
    uint32_t currentMsec;
    uint32_t hours;
    uint32_t minutes;
    R4A_ROBOT_CHALLENGE * previousChallenge;
    uint32_t seconds;

    // Only initialize the robot once
    previousChallenge = (R4A_ROBOT_CHALLENGE *)_challenge;
    if (previousChallenge)
    {
        display->printf("ERROR: Robot already running %s!", previousChallenge->name());
        return false;
    }

    // Synchronize with the stop routine
    _busy = true;

    // Compute the times for the challenge
    currentMsec = millis();
    _idleMsec = 0;
    _initMsec = currentMsec;
    _nextDisplayMsec = currentMsec;
    _startMsec = _initMsec + _startDelayMsec;
    _endMsec = _startMsec + (duration * R4A_MILLISECONDS_IN_A_SECOND);

    // Display the start delay time
    display->printf("Delaying %ld seconds before starting %s\r\n",
                    _startDelayMsec / 1000, challenge->name());

    // Split the duration
    seconds = duration;
    minutes = seconds / 60;
    seconds -= minutes * 60;
    hours = minutes / 60;
    minutes -= hours * 60;

    // Display the time
    display->printf("%s challenge duration %ld:%02ld:%02ld\r\n",
                    challenge->name(), hours, minutes, seconds);
    if (_displayTime)
        _displayTime(_startMsec - _nextDisplayMsec);

    // Call the initialization routine
    challenge->init();

    // Start the robot
    _challenge = challenge;    // Update the LED colors
    r4aLEDUpdate(true);

    SWITCH_STATE(STATE_COUNT_DOWN);

    // Release the synchronation with the stop routine
    _busy = false;
    return true;
}

//*********************************************************************
// Run the robot challenge
void R4A_ROBOT::running(uint32_t currentMsec)
{
    R4A_ROBOT_CHALLENGE * challenge;

    // Synchronize with the stop routine
    _busy = true;

    // Is the robot challenge still running
    challenge = (R4A_ROBOT_CHALLENGE *)_challenge;
    if (challenge)
    {
        // Determine the challenge should stop
        if (((int32_t)(_endMsec - currentMsec)) > 0)
            // Perform the robot challenge
            challenge->challenge();
        else
            // Stop the robot
            stop(currentMsec);
    }

    // Release the synchronation with the stop routine
    _busy = false;
}

//*********************************************************************
// Stop the robot
void R4A_ROBOT::stop(uint32_t currentMsec, Print * display)
{
    R4A_ROBOT_CHALLENGE * challenge;
    uint32_t hours;
    uint32_t milliseconds;
    uint32_t minutes;
    uint32_t seconds;
    uint8_t state;

    // Stop the robot just once by setting _state to STATE_STOP
    state = SWITCH_STATE(STATE_STOP);
    if ((state == STATE_RUNNING) || (state == STATE_COUNT_DOWN))
    {
        _stopMsec = currentMsec;

        // Wait for the I2C bus to be free, robot._core 0 idle
        if (_core != xPortGetCoreID())
            while (_busy)
            {
            }

        // Call the challenge stop routine to stop the motors
        challenge = (R4A_ROBOT_CHALLENGE *)_challenge;
        challenge->stop();

        // Display the runtime
        if (display)
        {
            // Split the runtime
            milliseconds = currentMsec - _startMsec;
            seconds = milliseconds / R4A_MILLISECONDS_IN_A_SECOND;
            milliseconds -= seconds * R4A_MILLISECONDS_IN_A_SECOND;
            minutes = seconds / R4A_SECONDS_IN_A_MINUTE;
            seconds -= minutes * R4A_SECONDS_IN_A_MINUTE;
            hours = minutes / R4A_MINUTES_IN_AN_HOUR;
            minutes -= hours * R4A_MINUTES_IN_AN_HOUR;
            display->printf("Stopped %s, runtime: %ld:%02ld:%02ld.%03ld\r\n",
                            challenge->name(), hours, minutes, seconds, milliseconds);
        }

        // Display the runtime
        if (_displayTime)
        _displayTime(_stopMsec - _startMsec);

        // Done with this challenge
        _challenge = nullptr;
    }
}


//*********************************************************************
// Wait after stopping the robot before switching to idle
void R4A_ROBOT::stopped(uint32_t currentMsec)
{
    // Initialize the delay
    if (_idleMsec == 0)
        _idleMsec = currentMsec;

    // Delay for a while
    if ((currentMsec - _stopMsec) >= _afterRunMsec)
    {
        r4aLEDsOff();
        SWITCH_STATE(STATE_IDLE);
    }
}

//*********************************************************************
// Update the robot state
void R4A_ROBOT::update(uint32_t currentMsec)
{
    uint8_t state;

    // Process the robot state
    state = _state;
    if (state == STATE_RUNNING)
        running(currentMsec);
    else if (state == STATE_COUNT_DOWN)
        initialDelay(currentMsec);
    else if (state == STATE_STOP)
        stopped(currentMsec);
    else if (state == STATE_IDLE)
    {
        if (_idle)
            _idle(currentMsec);
    }
    else
    {
        Serial.printf("ERROR: Unknown robot state %d\r\n", state);
        r4aReportFatalError("Unknown robot state");
    }
}
