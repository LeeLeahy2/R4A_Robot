/*------------------------------------------------------------------------------
NTRIP_Client.cpp

  Robots-For-All (R4A)
  NTRIP client to download GNSS corrections from an NTRIP server

  Based on code from SparkFun RTK EVK
  https://github.com/sparkfun/SparkFun_RTK_Everywhere_Firmware

  The NTRIP client sits on top of the WiFi layer and receives correction data
  from an NTRIP caster.  The correction data is then fed to the ZED F9P to
  enable centimeter position accuracy.

                Satellite     ...    Satellite
                     |         |          |
                     |         |          |
                     |         V          |
                     |        RTK         |
                     '------> Base <------'
                            Station
                               |
                               | NTRIP Server sends correction data
                               V
                         NTRIP Caster
                               |
                               | NTRIP Client receives correction data
                               V
                       Car (NTRIP Client)
                               |
                               | Correction data
                               V
                            ZED F9P

  Possible NTRIP Casters

    * https://emlid.com/ntrip-caster/
    * http://rtk2go.com/
    * private SNIP NTRIP caster
------------------------------------------------------------------------------*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  NTRIP Client States:
    NTRIP_CLIENT_OFF: NTRIP client disabled or missing parameters
    NTRIP_CLIENT_NETWORK_STARTED: Connecting to the network
    NTRIP_CLIENT_NETWORK_CONNECTED: Connected to the network
    NTRIP_CLIENT_CONNECTING: Attempting a connection to the NTRIP caster
    NTRIP_CLIENT_WAIT_RESPONSE: Wait for a response from the NTRIP caster
    NTRIP_CLIENT_HANDLE_RESPONSE: Process the response
    NTRIP_CLIENT_CONNECTED: Connected to the NTRIP caster

                               NTRIP_CLIENT_OFF <-------------.
                                       |                      |
                                 start |                      |
                                       v                      |
                          NTRIP_CLIENT_WAIT_FOR_WIFI          |
                                       |                      |
                                       |                      |
                                       v                Fail  |
                           NTRIP_CLIENT_CONNECTING ---------->+
                                       |                      ^
                                       |                      |
                                       v                Fail  |
                          NTRIP_CLIENT_WAIT_RESPONSE -------->+
                                       |                      ^
                                       |                      |
                                       v                Fail  |
                        NTRIP_CLIENT_HANDLE_RESPONSE -------->+
                                       |                      ^
                                       |                      |
                                       v                Fail  |
                            NTRIP_CLIENT_CONNECTED -----------'

  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#include "R4A_Robot.h"

//****************************************
// NTRIP Caster
//****************************************

const char * r4aNtripClientCasterHost = "rtk2go.com";           // NTRIP caster host name
const char * r4aNtripClientCasterMountPoint = "bldr_SparkFun1"; // NTRIP caster mount point
uint16_t r4aNtripClientCasterPort = 2101;                       // Typically 2101
const char * r4aNtripClientCasterUser = "your_email_address";   // e-mail address
const char * r4aNtripClientCasterUserPW = "";

const char * r4aNtripClientCompany = "Your name";       // "Company" for your robot
const char * r4aNtripClientProduct = "Your robot name"; // "Product" for your robot
const char * r4aNtripClientProductVersion = "0.0.1";    // "Version" of your robot

uint32_t r4aNtripClientReceiveTimeout =  60 * 1000; // mSec timeout for beginning of HTTP response from NTRIP caster
uint32_t r4aNtripClientResponseDone =         1000; // mSec timeout waiting for end of HTTP response from NTRIP caster
uint32_t r4aNtripClientResponseTimeout = 10 * 1000; // mSec timeout for RTCM data from NTRIP caster

volatile bool r4aNtripClientDebugRtcm = false;      // Display RTCM data
volatile bool r4aNtripClientDebugState = false;     // Display NTRIP client state transitions
volatile bool r4aNtripClientEnable = false;         // Enable the NTRIP client
volatile bool r4aNtripClientForcedShutdown = false; // Enable the NTRIP client

//*********************************************************************
// Attempt to connect to the NTRIP caster
bool R4A_NTRIP_CLIENT::connect()
{
    int bytesRemaining;
    Print * display;
    int length;

    if (!_client)
        return false;

    // Connect to the NTRIP caster
    display = getSerial();
    if (r4aNtripClientDebugState)
        display->printf("NTRIP Client connecting to %s:%d\r\n", r4aNtripClientCasterHost,
                        r4aNtripClientCasterPort);
    int connectResponse = _client->connect(r4aNtripClientCasterHost, r4aNtripClientCasterPort);
    if (connectResponse < 1)
    {
        if (r4aNtripClientDebugState)
            display->printf("NTRIP Client connection to NTRIP caster %s:%d failed\r\n",
                            r4aNtripClientCasterHost, r4aNtripClientCasterPort);
        return false;
    }

    // Set up the credentials
    const char * formatString;
    char * credentials;
    if (strlen(r4aNtripClientCasterUser) == 0)
    {
        formatString = "Accept: */*\r\nConnection: close";
        length = strlen(formatString) + 1;
        credentials = (char *)alloca(length);
        strcpy(credentials, formatString);
    }
    else
    {
        // Pass base64 encoded user:pw
        length = strlen(r4aNtripClientCasterUser) + 1 + strlen(r4aNtripClientCasterUserPW) + 1;
        char userCredentials[length];
        snprintf(userCredentials, length, "%s:%s", r4aNtripClientCasterUser, r4aNtripClientCasterUserPW);

        if (r4aNtripClientDebugState)
            display->printf("NTRIP Client sending credentials: %s\r\n", userCredentials);

        // Encode with ESP32 built-in library
        base64 b;
        String strEncodedCredentials = b.encode(userCredentials);
        length = strEncodedCredentials.length() + 1;
        char encodedCredentials[length];
        strEncodedCredentials.toCharArray(encodedCredentials, length); // Convert String to char array

        // Build the credentials
        formatString = "Authorization: Basic ";
        length = strlen(formatString) + strlen(strEncodedCredentials.c_str()) + 1;
        credentials = (char *)alloca(length);
        strcpy(credentials, formatString);
        strcat(credentials, strEncodedCredentials.c_str());
    }

    // Allocate the server request
    const char * getString = "GET /";
    const char * httpString = " HTTP/1.0";
    const char * agentString = "User-Agent: NTRIP ";
    length = strlen(getString) + strlen(r4aNtripClientCasterMountPoint) + strlen(httpString) + 2

           + strlen(agentString) + strlen(r4aNtripClientCompany) + 1
           + strlen(r4aNtripClientProduct) + 1 + strlen(r4aNtripClientProductVersion) + 2

           + strlen(credentials) + 2 + 2 + 1;
    char serverRequest[length];

    // Set up the server request (GET)
    strcpy(serverRequest, getString);
    strcat(serverRequest, r4aNtripClientCasterMountPoint);
    strcat(serverRequest, httpString);
    strcat(serverRequest, "\r\n");

    strcat(serverRequest, agentString);
    strcat(serverRequest, r4aNtripClientCompany);
    strcat(serverRequest, "_");
    strcat(serverRequest, r4aNtripClientProduct);
    strcat(serverRequest, "_");
    strcat(serverRequest, r4aNtripClientProductVersion);
    strcat(serverRequest, "\r\n");

    strcat(serverRequest, credentials);
    strcat(serverRequest, "\r\n");
    strcat(serverRequest, "\r\n");  // End of request

    if (r4aNtripClientDebugState)
        display->printf("NTRIP Client sending server request: %s", serverRequest);

    // Send the server request
    _client->write((const uint8_t *)serverRequest, strlen(serverRequest));
    return true;
}

//*********************************************************************
// Determine if another connection is possible or if the limit has been reached
bool R4A_NTRIP_CLIENT::connectLimitReached()
{
    int index;

    // Retry the connection a few times
    bool limitReached = (_connectionAttempts >= r4aNtripClientBbackoffCount);

    // Determine if there are any more connection attempts allowed
    if (limitReached)
        getSerial()->println("NTRIP Client connection attempts exceeded!");

    // Restart the NTRIP client
    stop(limitReached);
    return limitReached;
}

//*********************************************************************
void R4A_NTRIP_CLIENT::displayResponse(Print * display, char * response, int bytesRead)
{
    char byte;
    bool lineTerminationFound;
    const char * responseStart;

    // Walk through the response to locate each line
    while (((response - responseStart) < bytesRead) && *response)
    {
        // Locate the end of the line
        responseStart = response;
        lineTerminationFound = false;
        while (true)
        {
            byte = *response++;
            if ((byte == 0) || (byte == '\r') || (byte == '\n'))
            {
                lineTerminationFound = true;
                response[-1] = 0;
                break;
            }
        }

        // Remove the CR and LF
        while (true)
        {
            byte = *response;
            if ((byte == 0) || ((byte != '\r') && (byte != '\n')))
                break;
            response++;
        }

        // Output this line
        if (lineTerminationFound)
            display->printf("%s\r\n", responseStart);
        else
            display->printf("%s", responseStart);
    }
}

//*********************************************************************
// NTRIP Client was turned off due to an error. Don't allow restart!
void R4A_NTRIP_CLIENT::forceShutdown()
{
    r4aNtripClientForcedShutdown = true;
    stop(true);
}

//*********************************************************************
// Print the NTRIP client state summary
void R4A_NTRIP_CLIENT::printStateSummary(Print * display)
{
    switch (_state)
    {
    default:
        display->printf("Unknown: %d", _state);
        break;
    case NTRIP_CLIENT_OFF:
        if (!r4aNtripClientEnable)
            display->print("Disabled");
        else if (r4aNtripClientForcedShutdown)
            display->print("Disabled, error detected, forced shutdown");
        else if (!r4aNtripClientCasterHost)
            display->print("Disabled, NtripClientCasterHost not set!");
        else if (!r4aNtripClientCasterMountPoint)
            display->print("Disabled, NtripClientCasterMountPoint not set!");
        else if (!r4aNtripClientCasterUser)
            display->print("Disabled, NtripClientCasterUser not set!");
        else
            display->print("Disconnected");
        break;
    case NTRIP_CLIENT_WAIT_FOR_WIFI:
    case NTRIP_CLIENT_CONNECTING:
    case NTRIP_CLIENT_WAIT_RESPONSE:
        display->print("Connecting");
        break;
    case NTRIP_CLIENT_CONNECTED:
        display->print("Connected");
        break;
    }
}

//*********************************************************************
// Print the NTRIP Client status
void R4A_NTRIP_CLIENT::printStatus(Print * display)
{
    uint32_t days;
    byte hours;
    uint64_t milliseconds;
    byte minutes;
    byte seconds;

    // Display NTRIP Client status and uptime
    display->print("NTRIP Client ");
    if (!r4aNtripClientEnable)
        display->println("disabled!");
    else
    {
        printStateSummary(display);
        display->printf(" - %s/%s:%d", r4aNtripClientCasterHost,
                        r4aNtripClientCasterMountPoint, r4aNtripClientCasterPort);

        if (_state == NTRIP_CLIENT_CONNECTED)
            // Use _timer since it gets reset after each successful data
            // receiption from the NTRIP caster
            milliseconds = _timer - _startTime;
        else
        {
            milliseconds = _startTime;
            display->print(" Last");
        }

        // Display the uptime
        days = milliseconds / R4A_MILLISECONDS_IN_A_DAY;
        milliseconds %= R4A_MILLISECONDS_IN_A_DAY;

        hours = milliseconds / R4A_MILLISECONDS_IN_AN_HOUR;
        milliseconds %= R4A_MILLISECONDS_IN_AN_HOUR;

        minutes = milliseconds / R4A_MILLISECONDS_IN_A_MINUTE;
        milliseconds %= R4A_MILLISECONDS_IN_A_MINUTE;

        seconds = milliseconds / R4A_MILLISECONDS_IN_A_SECOND;
        milliseconds %= R4A_MILLISECONDS_IN_A_SECOND;

        display->print(" Uptime: ");
        display->printf("%d %02d:%02d:%02d.%03lld (Reconnects: %d)\r\n",
                        days, hours, minutes, seconds, milliseconds,
                        _connectionAttemptsTotal);
    }
}

//*********************************************************************
// Add data to the ring buffer
int R4A_NTRIP_CLIENT::rbAddData(int length)
{
    int bytesFree;
    int bytesToCopy;
    int bytesRead;
    int bytesToTail;
    int bytesWritten;
    int totalBytesToCopy;

    //           Tail --.                    .-- Head
    //                  |                    |
    //    Start         V                    V                 End
    //      |           DDDDDDDDDDDDDDDDDDDDD                   |
    //      +---------------------------------------------------+
    //      |ddddddd                                 ddddddddddd|
    //              ^                                ^
    //              |                                |
    //            Head                              Tail
    //
    // The maximum transfer only fills the free space in the ring buffer
    // * tail --> head
    bytesWritten = 0;
    bytesFree = _rbTail - _rbHead
              + (R4A_NTRIP_CLIENT_RING_BUFFER_BYTES << 1) - 1;
    bytesFree %= R4A_NTRIP_CLIENT_RING_BUFFER_BYTES;
    if (bytesFree)
    {
        if (length > bytesFree)
            length = bytesFree;

        // Copy all of the data if possible
        totalBytesToCopy = length;

        // Limit the transfer size to the contiguous space in the buffer
        // * head --> end-of-buffer
        bytesToCopy = totalBytesToCopy;
        bytesToTail = R4A_NTRIP_CLIENT_RING_BUFFER_BYTES - _rbHead;
        if (bytesToCopy > bytesToTail)
            bytesToCopy = bytesToTail;

        // Fill the contiguous portion of the buffer
        totalBytesToCopy -= bytesToCopy;
        bytesRead = _client->read((uint8_t *)&_ringBuffer[_rbHead],
                                   bytesToCopy);

        // Fill the remaining portion of the buffer
        if (totalBytesToCopy)
            bytesRead += _client->read((uint8_t *)_ringBuffer,
                                       totalBytesToCopy);

        // Display the bytes read
        if (r4aNtripClientDebugRtcm)
            Serial.printf("NTRIP RX --> buffer, %d RTCM bytes.\r\n", bytesRead);

        // Account for the data copied
        bytesWritten += bytesRead;
        _rbHead += bytesRead;
        if (_rbHead >= R4A_NTRIP_CLIENT_RING_BUFFER_BYTES)
            _rbHead -= R4A_NTRIP_CLIENT_RING_BUFFER_BYTES;
    }

    // Determine if any data was read from the NTRIP caster
    if (bytesWritten)
        // Restart the NTRIP receive data timer
        _timer = millis();
    return bytesWritten;
}

//*********************************************************************
// Remove data from the ring buffer
int R4A_NTRIP_CLIENT::rbRemoveData(Print * display)
{
    int bytesAvailable;

    int bytesRead;
    int bytesToTail;
    int bytesMinimum;
    int bytesToPush;
    int bytesWritten;
    uint8_t transactionSize;

    // NTRIP client must be actively receiving data
    if (_state != NTRIP_CLIENT_CONNECTED)
        return 0;

    //           Tail --.                    .-- Head
    //                  |                    |
    //    Start         V                    V                 End
    //      |           DDDDDDDDDDDDDDDDDDDDD                   |
    //      +---------------------------------------------------+
    //      |ddddddd                                 ddddddddddd|
    //              ^                                ^
    //              |                                |
    //            Head                              Tail
    //
    // Determine the number of bytes in the ring buffer
    bytesAvailable = _rbHead + R4A_NTRIP_CLIENT_RING_BUFFER_BYTES
                   - _rbTail;
    bytesAvailable %= R4A_NTRIP_CLIENT_RING_BUFFER_BYTES;

    // Get the I2C transaction size
    transactionSize = _i2cTransactionSize;

    // Wait for more data if necessary
    bytesWritten = 0;
    bytesMinimum = _minimumRxBytes << 1;
    if (bytesAvailable >= bytesMinimum)
    {
        while (bytesAvailable >= bytesMinimum)
        {
            // Copy all of the data if possible
            bytesToPush = bytesAvailable;

            // Limit this value to the contiguous data in the buffer
            bytesToTail = R4A_NTRIP_CLIENT_RING_BUFFER_BYTES - _rbTail;
            if (bytesToPush > bytesToTail)
                bytesToPush = bytesToTail;

            // Limit this to what the GNSS can receive
            if (bytesToPush > transactionSize)
                bytesToPush = transactionSize;

            //                         Tail --.                    .-- Head
            //                                |                    |
            //    Start                       V                    V   End
            //      |                         DDDDDDDDDDDDDDDDDDDDD     |
            //      +------------------------------------------+--------+
            //      |ddddddd                                 ddddddddddd|
            //              ^                                ^
            //              |                                |
            //            Head                              Tail
            //
            // The GNSS is documented to not like single byte transfers
            // Always reserve a small portion at the end of the buffer when
            // the data does not quite reach the end of the buffer
            bytesToTail -= bytesToPush;
            if (bytesToTail && (bytesToTail < _minimumRxBytes))
                bytesToPush -= _minimumRxBytes - bytesToTail;

            // Push data to the GNSS using I2C
            int bytesPushed = pushRawData((uint8_t *)&_ringBuffer[_rbTail],
                                          bytesToPush,
                                          display);
            if (bytesPushed == 0)
            {
                Serial.printf("NTRIP buffer --> GNSS failed! bytesWritten: %d\r\n", bytesWritten);
                break;
            }

            // Account for the data copied
            bytesAvailable -= bytesPushed;
            bytesWritten += bytesPushed;
            _rbTail += bytesPushed;
            if (_rbTail >= R4A_NTRIP_CLIENT_RING_BUFFER_BYTES)
                _rbTail -= R4A_NTRIP_CLIENT_RING_BUFFER_BYTES;
        }

        // Display the RTCM written to the GNSS
        if (r4aNtripClientDebugRtcm)
            Serial.printf("NTRIP buffer --> GNSS, %d RTCM bytes.\r\n", bytesWritten);
    }

    // Return the number of byte written
    return bytesWritten;
}

//*********************************************************************
// Read the response from the NTRIP client
void R4A_NTRIP_CLIENT::response(Print *display, int length)
{
    int bytesRead;
    char data[R4A_NTRIP_CLIENT_RESPONSE_BUFFER_SIZE];
    char *response;

    // Read bytes from the NTRIP caster
    while (length > 0)
    {
        // Get the buffer address
        response = _responseLength ? data : _responseBuffer;

        // Don't overfill the buffer
        if (length > (R4A_NTRIP_CLIENT_RESPONSE_BUFFER_SIZE - 1))
            length = R4A_NTRIP_CLIENT_RESPONSE_BUFFER_SIZE - 1;

        // Get the NTRIP caster data
        bytesRead = _client->read((uint8_t *)response, length);

        // Zero terminate the response
        response[bytesRead] = 0;

        // Save the length of the first packet of the response
        if (!_responseLength)
            _responseLength = bytesRead;

        // Extend the end-of-response timeout
        _timer = millis();

        // Display the response if necessary
        if (r4aNtripClientDebugState)
            display->print(response);

        // Account for the data
        length -= bytesRead;
    }
}

//*********************************************************************
// Restart the NTRIP client
void R4A_NTRIP_CLIENT::restart()
{
    // Save the previous uptime value
    if (_state == NTRIP_CLIENT_CONNECTED)
        _startTime = _timer - _startTime;
    connectLimitReached();
}

//*********************************************************************
// Update the state of the NTRIP client state machine
void R4A_NTRIP_CLIENT::setState(uint8_t newState)
{
    Print * display;

    if (!r4aNtripClientDebugState)
        _state = newState;
    else
    {
        display = getSerial();
        if (_state == newState)
            display->print("NTRIP Client: *");
        else
            display->printf("NTRIP Client: %s --> ", r4aNtripClientStateName[_state]);
        _state = newState;
        if (newState >= NTRIP_CLIENT_STATE_MAX)
        {
            display->printf("Unknown client state: %d\r\n", newState);
            r4aReportFatalError("Unknown NTRIP Client state");
        }
        else
            display->println(r4aNtripClientStateName[_state]);
    }
}

//*********************************************************************
// Start the NTRIP client
void R4A_NTRIP_CLIENT::start()
{
    // Get the transaction size
    _i2cTransactionSize = i2cTransactionSize();

    // Start the NTRIP client
    stop(false);
}

//*********************************************************************
// Shutdown or restart the NTRIP client
bool R4A_NTRIP_CLIENT::stop(bool shutdown)
{
    Print * display;
    bool stopped;

    // Release the previous client connection
    if (_client)
    {
        // Break the NTRIP client connection if necessary
        if (_client->connected())
            _client->stop();

        // Free the NTRIP client resources
        delete _client;
        _client = nullptr;
    }

    // Determine the next NTRIP client state
    display = getSerial();
    stopped = shutdown || (!r4aNtripClientEnable);
    if (stopped)
    {
        // Set the next NTRIP client state
        if (_state != NTRIP_CLIENT_OFF)
        {
            setState(NTRIP_CLIENT_OFF);
            _connectionAttempts = 0;
            display->println("NTRIP Client stopped");
        }
    }
    else
    {
        // Get the backoff time in milliseconds
        int index = _connectionAttempts;
        if (index >= r4aNtripClientBbackoffCount)
            index = r4aNtripClientBbackoffCount - 1;
        _connectionDelayMsec = r4aNtripClientBbackoffIntervalMsec[index];

        // Start the backoff timer now, overlay delay with WiFi connection
        _timer = millis();

        // Display the delay before starting the NTRIP client
        if (r4aNtripClientDebugState)
        {
            Print * display = getSerial();
            if (_connectionAttempts)
                display->print("NTRIP Client starting");
            else
                display->print("NTRIP Client trying again");
            if (!_connectionDelayMsec)
                display->println();
            else
            {
                int seconds = _connectionDelayMsec / 1000;
                if (seconds <= 60)
                    display->printf(" in %d seconds.\r\n", seconds);
                else
                    display->printf(" in %d minutes.\r\n", seconds / 60);
            }
        }

        // Set the next NTRIP client state
        setState(NTRIP_CLIENT_WAIT_FOR_WIFI);
    }

    // Return the stopped state
    return stopped;
}

//*********************************************************************
// Check for the arrival of any correction data. Push it to the GNSS.
// Stop task if the connection has dropped or if we receive no data for
// _receiveTimeout
void R4A_NTRIP_CLIENT::update(bool wifiConnected)
{
    Print * display = getSerial();

    // Shutdown the NTRIP client when the mode or setting changes
    if ((!r4aNtripClientEnable) && (_state > NTRIP_CLIENT_OFF))
        stop(true);

    // Enable the network and the NTRIP client if requested
    switch (_state)
    {
    // NTRIP client disabled or missing a parameter
    case NTRIP_CLIENT_OFF:
        // Wait until the NTRIP client is enabled
        if (r4aNtripClientEnable)
        {
            // Don't allow the client to restart if a forced shutdown occured
            if (r4aNtripClientForcedShutdown)
                display->println("ERROR: Please clear the forced error before starting the NTRIP client!");
            else if (!r4aNtripClientCasterHost)
                display->println("ERROR: Please set the NTRIP caster host name!");
            else if (!r4aNtripClientCasterMountPoint)
                display->println("ERROR: Please set the NTRIP caster mount point!");
            else if (!r4aNtripClientCasterUser)
                display->println("ERROR: Please set the NTRIP caster user name!");
            else
            {
                // NTRIP client is enabled with all the necessary paramters
                start();
                break;
            }
            r4aNtripClientEnable = false;
            break;
        }
        break;

    // Wait for a network media connection
    case NTRIP_CLIENT_WAIT_FOR_WIFI:
        // Determine if the host is connected via WiFi
        if (wifiConnected)
        {
            // Allocate the _client structure
            _client = new NetworkClient();
            if (!_client)
            {
                // Failed to allocate the _client structure
                display->println("ERROR: Failed to allocate the _client structure!");
                forceShutdown();
            }
            else
            {
                // Account for this connection attempt
                _connectionAttempts += 1;
                _connectionAttemptsTotal += 1;

                // The network is available for the NTRIP client
                setState(NTRIP_CLIENT_CONNECTING);
            }
        }
        break;

    case NTRIP_CLIENT_CONNECTING:
        // Determine if the WiFi link is working
        if (!wifiConnected)
            // Wifi link failed, retry the NTRIP client connection
            restart();

        // Delay before opening the NTRIP client connection
        else if ((millis() - _timer) >= _connectionDelayMsec)
        {
            // Open connection to NTRIP caster service
            if (!connect())
            {
                // Assume service not available
                if (connectLimitReached()) // Updates _connectionDelayMsec
                    display->println(
                        "NTRIP caster failed to connect. Do you have your caster address and port correct?");
            }
            else
            {
                // Socket opened to NTRIP system
                if (r4aNtripClientDebugState)
                    display->printf("NTRIP Client waiting for response from %s:%d\r\n",
                                    r4aNtripClientCasterHost, r4aNtripClientCasterPort);

                // Start the response timer
                _timer = millis();
                _responseLength = 0;
                setState(NTRIP_CLIENT_WAIT_RESPONSE);
            }
        }
        break;

    case NTRIP_CLIENT_WAIT_RESPONSE:
        // Determine if the WiFi link is working
        if (!wifiConnected)
            // Wifi link failed, retry the NTRIP client connection
            restart();

        // At least a few bytes received, wait ultil the response is done
        else
        {
            // Save the initial portion of the response
            int length = _client->available();
            if (length)
            {
                // Check for the end of the response
                if (_client->peek() == 0xd3)
                    // End of response
                    setState(NTRIP_CLIENT_HANDLE_RESPONSE);

                // Get the next portion of the response
                else
                    response(display, length);
            }

            // Check for the end of the response timeout
            else if ((millis() - _timer) >= r4aNtripClientResponseDone)
                // End of response
                setState(NTRIP_CLIENT_HANDLE_RESPONSE);

            // Check for a response timeout
            else if ((!_responseLength)
                && ((millis() - _timer) > r4aNtripClientResponseTimeout))
            {
                // NTRIP web service did not respond
               if (connectLimitReached()) // Updates _connectionDelayMsec
                    display->println("NTRIP Caster failed to respond. Do you have your caster address and port correct?");
            }
        }
        break;

    case NTRIP_CLIENT_HANDLE_RESPONSE:
        // Determine if the WiFi link is working
        if (!wifiConnected)
            // Wifi link failed, retry the NTRIP client connection
            restart();

        // Process the respones
        else
        {
            const char * response = _responseBuffer;

            // Separate the output
            if (r4aNtripClientDebugState)
                display->println();

            // Look for various responses
            if (strstr(response, "200") != nullptr) //'200' found
            {
                // Check for banned from the NTRIP caster
                if (strcasestr(response, "banned") != nullptr)
                {
                    display->printf("NTRIP Client banned from %s!\r\n",
                                    r4aNtripClientCasterHost);

                    // No longer allowed to access this website
                    // Try again later
                    forceShutdown();
                }

                // Check for using sandbox
                else if (strcasestr(response, "sandbox") != nullptr)
                {
                    display->printf("NTRIP Client redirected to sandbox on %s!\r\n",
                                    r4aNtripClientCasterHost);

                    connectLimitReached(); // Re-attempted after a period of time. Shuts down NTRIP Client if
                                                      // limit reached.
                }

                // Check for failed lookup
                else if (strcasestr(response, "SOURCETABLE") != nullptr)
                {
                    display->printf("Mount point %s not found on %s!\r\n",
                                    r4aNtripClientCasterMountPoint, r4aNtripClientCasterHost);

                    // Stop NTRIP client operations
                    forceShutdown();
                }
                else
                {
                    // Timeout receiving NTRIP data, retry the NTRIP client connection
                    #if USING_NTP
                        display->printf("NTRIP Client connected to %s:%d at %s\r\n",
                                        r4aNtripClientCasterHost, r4aNtripClientCasterPort,
                                        ntpGetTime24(ntpGetEpochTime()));
                    #else USING_NTP
                        display->printf("NTRIP Client connected to %s:%d\r\n",
                                        r4aNtripClientCasterHost, r4aNtripClientCasterPort);
                    #endif  // USING_NTP

                    // Connection is now open, start the NTRIP receive data timer
                    _startTime = millis();
                    _timer = _startTime;

                    // Start processing correction data
                    setState(NTRIP_CLIENT_CONNECTED);
                }
            }

            // Check for unauthorized user
            else if (strstr(response, "401") != nullptr)
            {
                // Look for '401 Unauthorized'
                display->printf("User %s not authorized on NTRIP Caster %s!\r\n"
                                "Are you sure your caster credentials are correct?\r\n",
                                r4aNtripClientCasterUser, r4aNtripClientCasterHost);

                // Stop NTRIP client operations
               forceShutdown();
            }

            // Check for startup phase
            else if (strstr(response, "406") != nullptr)
            {
                // Look for '406 In Start Up Phase'
                display->printf("NTRIP caster %s is in its startup phase!\r\n",
                                r4aNtripClientCasterHost);

                // Stop NTRIP client operations
                restart();
            }

            // Other errors returned by the caster
            else
            {
                if (!r4aNtripClientEnable)
                    display->print(_responseBuffer);
                if (_responseLength)
                {
                    display->printf("NTRIP caster %s responded with an error!\r\n",
                                    r4aNtripClientCasterHost);

                    // Stop NTRIP client operations
                    forceShutdown();
                }
                else
                {
                    display->printf("Response timeout from NTRIP caster %s!\r\n",
                                    r4aNtripClientCasterHost);
                    connectLimitReached(); // Re-attempted after a period of time. Shuts down NTRIP Client if
                                           // limit reached.
                }
            }
        }
        break;

    case NTRIP_CLIENT_CONNECTED:
        // Determine if the WiFi link is working
        if (!wifiConnected)
            // Wifi link failed, retry the NTRIP client connection
            restart();

        // Check for a broken connection
        else if (!_client->connected())
        {
            // Broken connection, retry the NTRIP client connection
            display->println("NTRIP Client connection to caster was broken");
            restart();
        }
        else
        {
            // Handle other types of NTRIP connection failures to prevent
            // hammering the NTRIP caster with rapid connection attempts.
            // A fast reconnect is reasonable after a long NTRIP caster
            // connection.  However increasing backoff delays should be
            // added when the NTRIP caster fails after a short connection
            // interval.
            if (_connectionAttempts
                && ((millis() - _startTime) > R4A_NTRIP_CLIENT_CONNECTION_TIME))
            {
                // After a long connection period, reset the attempt counter
                _connectionAttempts = 0;
                if (r4aNtripClientDebugState)
                    display->println("NTRIP Client resetting connection attempt counter and timeout");
            }

            // Check for timeout receiving NTRIP data
            if (_client->available() == 0)
            {
                // Don't fail during retransmission attempts
                if ((millis() - _timer) > r4aNtripClientReceiveTimeout)
                {
                    // Timeout receiving NTRIP data, retry the NTRIP client connection
                    #if USING_NTP
                        display->printf("NTRIP Client timeout receiving data at %s\r\n",
                                        ntpGetTime24(ntpGetEpochTime()));
                    #else USING_NTP
                        display->println("NTRIP Client timeout receiving data");
                    #endif  // USING_NTP
                    restart();
                }
            }
            else
            {
                int availableBytes;

                // Receive data from the NTRIP Caster
                availableBytes = _client->available();
                while (availableBytes)
                {
                    rbAddData(availableBytes);
                    availableBytes = _client->available();
                }
            }
        }
        break;
    }
}

//*********************************************************************
// Verify the NTRIP client tables
void R4A_NTRIP_CLIENT::validateTables()
{
    if (r4aNtripClientStateNameEntries != NTRIP_CLIENT_STATE_MAX)
        r4aReportFatalError("Fix r4aNtripClientStateNameEntries to match _state");
}
