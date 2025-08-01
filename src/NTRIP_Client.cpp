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
// Constants
//****************************************

// Define the NTRIP client states
enum r4aNtripClientState
{
    NTRIP_CLIENT_OFF = 0,           // NTRIP client disabled or missing parameters
    NTRIP_CLIENT_WAIT_FOR_WIFI,     // Connecting to WiFi access point
    NTRIP_CLIENT_CONNECTING,        // Attempting a connection to the NTRIP caster
    NTRIP_CLIENT_WAIT_RESPONSE,     // Wait for a response from the NTRIP caster
    NTRIP_CLIENT_HANDLE_RESPONSE,   // Process the response
    NTRIP_CLIENT_CONNECTED,         // Connected to the NTRIP caster

    // Insert new states above this line
    NTRIP_CLIENT_STATE_MAX // Last entry in the state list
};

const char *const r4aNtripClientStateName[] = {"NTRIP_CLIENT_OFF",
                                            "NTRIP_CLIENT_WAIT_FOR_WIFI",
                                            "NTRIP_CLIENT_CONNECTING",
                                            "NTRIP_CLIENT_WAIT_RESPONSE",
                                            "NTRIP_CLIENT_HANDLE_RESPONSE",
                                            "NTRIP_CLIENT_CONNECTED"};
const int r4aNtripClientStateNameEntries = sizeof(r4aNtripClientStateName) / sizeof(r4aNtripClientStateName[0]);

//****************************************
// NTRIP Client parameters
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

//****************************************
// Globals
//****************************************

R4A_NTRIP_CLIENT r4aNtripClient;

//****************************************
// Forward routine declarations
//****************************************

bool r4aNtripClientStop(bool shutdown, Print * display);

//*********************************************************************
// Attempt to connect to the NTRIP caster
bool r4aNtripClientConnect(Print * display)
{
    int length;

    if (!r4aNtripClient._client)
        return false;

    // Connect to the NTRIP caster
    if (display)
        display->printf("NTRIP Client connecting to %s:%d\r\n", r4aNtripClientCasterHost,
                        r4aNtripClientCasterPort);
    int connectResponse = r4aNtripClient._client->connect(r4aNtripClientCasterHost, r4aNtripClientCasterPort);
    if (connectResponse < 1)
    {
        if (display)
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

        if (display)
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

    if (display)
        display->printf("NTRIP Client sending server request: %s", serverRequest);

    // Send the server request
    r4aNtripClient._client->write((const uint8_t *)serverRequest, strlen(serverRequest));
    return true;
}

//*********************************************************************
// Determine if another connection is possible or if the limit has been reached
bool r4aNtripClientConnectLimitReached(Print * display)
{
    // Retry the connection a few times
    bool limitReached = (r4aNtripClient._connectionAttempts >= r4aNtripClientBbackoffCount);

    // Determine if there are any more connection attempts allowed
    if (limitReached)
    {
        Print * errorPrint = display ? display : &Serial;
        errorPrint->println("NTRIP Client connection attempts exceeded!");
    }

    // Restart the NTRIP client
    r4aNtripClientStop(limitReached, display);
    return limitReached;
}

//*********************************************************************
void r4aNtripClientDisplayResponse(char * response,
                                   int bytesRead,
                                   Print * display)
{
    char byte;
    bool lineTerminationFound;
    const char * responseStart;

    // Walk through the response to locate each line
    responseStart = response;
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
void r4aNtripClientForceShutdown(Print * display)
{
    r4aNtripClientForcedShutdown = true;
    r4aNtripClientStop(true, display);
}

//*********************************************************************
// Print the NTRIP client state summary
void r4aNtripClientPrintStateSummary(Print * display)
{
    switch (r4aNtripClient._state)
    {
    default:
        display->printf("Unknown: %d", r4aNtripClient._state);
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
void r4aNtripClientPrintStatus(Print * display)
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
        r4aNtripClientPrintStateSummary(display);
        display->printf(" - %s:%d/%s", r4aNtripClientCasterHost,
                        r4aNtripClientCasterPort, r4aNtripClientCasterMountPoint);

        if (r4aNtripClient._state == NTRIP_CLIENT_CONNECTED)
            // Use _timer since it gets reset after each successful data
            // receiption from the NTRIP caster
            milliseconds = r4aNtripClient._timer - r4aNtripClient._startTime;
        else
        {
            milliseconds = r4aNtripClient._startTime;
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
        display->printf("%ld %02d:%02d:%02d.%03lld (Reconnects: %d)\r\n",
                        days, hours, minutes, seconds, milliseconds,
                        r4aNtripClient._connectionAttemptsTotal);
    }
}

//*********************************************************************
// Add data to the ring buffer
int r4aNtripClientRbAddData(int length, Print * display)
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
    bytesFree = r4aNtripClient._rbTail - r4aNtripClient._rbHead
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
        bytesToTail = R4A_NTRIP_CLIENT_RING_BUFFER_BYTES - r4aNtripClient._rbHead;
        if (bytesToCopy > bytesToTail)
            bytesToCopy = bytesToTail;

        // Fill the contiguous portion of the buffer
        totalBytesToCopy -= bytesToCopy;
        bytesRead = r4aNtripClient._client->read((uint8_t *)&r4aNtripClient._ringBuffer[r4aNtripClient._rbHead],
                                                 bytesToCopy);

        // Fill the remaining portion of the buffer
        if (totalBytesToCopy)
            bytesRead += r4aNtripClient._client->read((uint8_t *)r4aNtripClient._ringBuffer,
                                                      totalBytesToCopy);

        // Display the bytes read
        Print * debugDisplay = r4aNtripClientDebugRtcm ? display : nullptr;
        if (debugDisplay)
            debugDisplay->printf("NTRIP RX --> buffer, %d RTCM bytes.\r\n", bytesRead);

        // Account for the data copied
        bytesWritten += bytesRead;
        r4aNtripClient._rbHead = r4aNtripClient._rbHead + bytesRead;
        if (r4aNtripClient._rbHead >= R4A_NTRIP_CLIENT_RING_BUFFER_BYTES)
            r4aNtripClient._rbHead = r4aNtripClient._rbHead - R4A_NTRIP_CLIENT_RING_BUFFER_BYTES;
    }

    // Determine if any data was read from the NTRIP caster
    if (bytesWritten)
        // Restart the NTRIP receive data timer
        r4aNtripClient._timer = millis();
    return bytesWritten;
}

//*********************************************************************
// Remove data from the ring buffer
int r4aNtripClientRbRemoveData(Print * display)
{
    int bytesAvailable;
    int bytesToTail;
    int bytesMinimum;
    int bytesToPush;
    int bytesWritten;
    Print * debugRtcm;
    Print * errorPrint;
    uint8_t transactionSize;

    // Determine the output locations
    debugRtcm = r4aNtripClientDebugRtcm ? display : nullptr;
    errorPrint = display ? display : &Serial;

    // NTRIP client must be actively receiving data
    if (r4aNtripClient._state != NTRIP_CLIENT_CONNECTED)
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
    bytesAvailable = r4aNtripClient._rbHead + R4A_NTRIP_CLIENT_RING_BUFFER_BYTES
                   - r4aNtripClient._rbTail;
    bytesAvailable %= R4A_NTRIP_CLIENT_RING_BUFFER_BYTES;

    // Get the I2C transaction size
    transactionSize = r4aNtripClient._i2cTransactionSize;

    // Wait for more data if necessary
    bytesWritten = 0;
    bytesMinimum = R4A_NTRIP_CLIENT_MINIMUM_RX_BYTES << 1;
    if (bytesAvailable >= bytesMinimum)
    {
        while (bytesAvailable >= bytesMinimum)
        {
            // Copy all of the data if possible
            bytesToPush = bytesAvailable;

            // Limit this value to the contiguous data in the buffer
            bytesToTail = R4A_NTRIP_CLIENT_RING_BUFFER_BYTES - r4aNtripClient._rbTail;
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
            if (bytesToTail && (bytesToTail < R4A_NTRIP_CLIENT_MINIMUM_RX_BYTES))
                bytesToPush -= R4A_NTRIP_CLIENT_MINIMUM_RX_BYTES - bytesToTail;

            // Push data to the GNSS using I2C
            int bytesPushed = r4aNtripClientPushRawData((uint8_t *)&r4aNtripClient._ringBuffer[r4aNtripClient._rbTail],
                                                        bytesToPush);
            if (bytesPushed == 0)
            {
                errorPrint->printf("NTRIP buffer --> GNSS failed! bytesWritten: %d\r\n", bytesWritten);
                break;
            }

            // Account for the data copied
            bytesAvailable -= bytesPushed;
            bytesWritten += bytesPushed;
            r4aNtripClient._rbTail = r4aNtripClient._rbTail + bytesPushed;
            if (r4aNtripClient._rbTail >= R4A_NTRIP_CLIENT_RING_BUFFER_BYTES)
                r4aNtripClient._rbTail = r4aNtripClient._rbTail - R4A_NTRIP_CLIENT_RING_BUFFER_BYTES;
        }

        // Display the RTCM written to the GNSS
        if (debugRtcm)
            debugRtcm->printf("NTRIP buffer --> GNSS, %d RTCM bytes.\r\n", bytesWritten);
    }

    // Return the number of byte written
    return bytesWritten;
}

//*********************************************************************
// Read the response from the NTRIP client
void r4aNtripClientResponse(Print *display, int length)
{
    int bytesRead;
    char data[R4A_NTRIP_CLIENT_RESPONSE_BUFFER_SIZE];
    char *response;

    // Read bytes from the NTRIP caster
    while (length > 0)
    {
        // Get the buffer address
        response = r4aNtripClient._responseLength ? data : r4aNtripClient._responseBuffer;

        // Don't overfill the buffer
        if (length > (R4A_NTRIP_CLIENT_RESPONSE_BUFFER_SIZE - 1))
            length = R4A_NTRIP_CLIENT_RESPONSE_BUFFER_SIZE - 1;

        // Get the NTRIP caster data
        bytesRead = r4aNtripClient._client->read((uint8_t *)response, length);

        // Zero terminate the response
        response[bytesRead] = 0;

        // Save the length of the first packet of the response
        if (!r4aNtripClient._responseLength)
            r4aNtripClient._responseLength = bytesRead;

        // Extend the end-of-response timeout
        r4aNtripClient._timer = millis();

        // Display the response if necessary
        if (r4aNtripClientDebugState)
            display->print(response);

        // Account for the data
        length -= bytesRead;
    }
}

//*********************************************************************
// Restart the NTRIP client
void r4aNtripClientRestart(Print * display)
{
    // Save the previous uptime value
    if (r4aNtripClient._state == NTRIP_CLIENT_CONNECTED)
        r4aNtripClient._startTime = r4aNtripClient._timer - r4aNtripClient._startTime;
    r4aNtripClientConnectLimitReached(display);
}

//*********************************************************************
// Update the state of the NTRIP client state machine
void r4aNtripClientSetState(uint8_t newState, Print * display)
{
    Print * debugState = r4aNtripClientDebugState ? display : nullptr;

    if (!debugState)
        r4aNtripClient._state = newState;
    else
    {
        if (r4aNtripClient._state == newState)
            debugState->print("NTRIP Client: *");
        else
            debugState->printf("NTRIP Client: %s --> ", r4aNtripClientStateName[r4aNtripClient._state]);
        r4aNtripClient._state = newState;
        if (newState >= NTRIP_CLIENT_STATE_MAX)
        {
            debugState->printf("Unknown client state: %d\r\n", newState);
            r4aReportFatalError("Unknown NTRIP Client state");
        }
        else
            debugState->println(r4aNtripClientStateName[r4aNtripClient._state]);
    }
}

//*********************************************************************
// Start the NTRIP client
void r4aNtripClientStart(Print * display)
{
    // Get the transaction size
    r4aNtripClient._i2cTransactionSize = r4aNtripClientI2cTransactionSize();

    // Start the NTRIP client
    r4aNtripClientStop(false, display);
}

//*********************************************************************
// Shutdown or restart the NTRIP client
bool r4aNtripClientStop(bool shutdown, Print * display)
{
    Print * debugState;
    Print * errorPrint;
    bool stopped;

    // Determine the output locations
    debugState = r4aNtripClientDebugState ? display : nullptr;
    errorPrint = display ? display : &Serial;

    // Release the previous client connection
    if (r4aNtripClient._client)
    {
        // Break the NTRIP client connection if necessary
        if (r4aNtripClient._client->connected())
            r4aNtripClient._client->stop();

        // Free the NTRIP client resources
        delete r4aNtripClient._client;
        r4aNtripClient._client = nullptr;
    }

    // Determine the next NTRIP client state
    stopped = shutdown || (!r4aNtripClientEnable);
    if (stopped)
    {
        // Set the next NTRIP client state
        if (r4aNtripClient._state != NTRIP_CLIENT_OFF)
        {
            r4aNtripClientSetState(NTRIP_CLIENT_OFF, display);
            r4aNtripClient._connectionAttempts = 0;
            errorPrint->println("NTRIP Client stopped");
        }
    }
    else
    {
        // Get the backoff time in milliseconds
        int index = r4aNtripClient._connectionAttempts;
        if (index >= r4aNtripClientBbackoffCount)
            index = r4aNtripClientBbackoffCount - 1;
        r4aNtripClient._connectionDelayMsec = r4aNtripClientBbackoffIntervalMsec[index];

        // Start the backoff timer now, overlay delay with WiFi connection
        r4aNtripClient._timer = millis();

        // Display the delay before starting the NTRIP client
        if (debugState)
        {
            if (r4aNtripClient._connectionAttempts)
                debugState->print("NTRIP Client starting");
            else
                debugState->print("NTRIP Client trying again");
            if (!r4aNtripClient._connectionDelayMsec)
                debugState->println();
            else
            {
                int seconds = r4aNtripClient._connectionDelayMsec / 1000;
                if (seconds <= 60)
                    debugState->printf(" in %d seconds.\r\n", seconds);
                else
                    debugState->printf(" in %d minutes.\r\n", seconds / 60);
            }
        }

        // Set the next NTRIP client state
        r4aNtripClientSetState(NTRIP_CLIENT_WAIT_FOR_WIFI, display);
    }

    // Return the stopped state
    return stopped;
}

//*********************************************************************
// Check for the arrival of any correction data. Push it to the GNSS.
// Stop task if the connection has dropped or if we receive no data for
// _receiveTimeout
void r4aNtripClientUpdate(bool wifiConnected, Print * display)
{
    Print * debugState;
    Print * errorPrint;
    int length;
    const char * response;

    // Determine the output locations
    debugState = r4aNtripClientDebugState ? display : nullptr;
    errorPrint = display ? display : &Serial;

    // Shutdown the NTRIP client when the mode or setting changes
    if ((!r4aNtripClientEnable) && (r4aNtripClient._state > NTRIP_CLIENT_OFF))
    {
        if (debugState)
            debugState->println("NTRIP Client: WiFi link to remote AP failed!");
        r4aNtripClientStop(true, display);
    }

    // Determine if the WiFi link is working
    if ((!wifiConnected) && (r4aNtripClient._state > NTRIP_CLIENT_WAIT_FOR_WIFI))
    {
        // Wifi link failed, retry the NTRIP client connection
        if (debugState)
            debugState->println("NTRIP Client: WiFi link to remote AP failed!");
        r4aNtripClientRestart(display);
    }

    // Check for a broken connection
    else if ((r4aNtripClient._state > NTRIP_CLIENT_WAIT_RESPONSE)
        && (!r4aNtripClient._client->connected()))
    {
        // Broken connection, retry the NTRIP client connection
        if (debugState)
            debugState->println("NTRIP Client connection to caster was broken!");
        r4aNtripClientRestart(display);
    }

    // Enable the network and the NTRIP client if requested
    switch (r4aNtripClient._state)
    {
    // NTRIP client disabled or missing a parameter
    case NTRIP_CLIENT_OFF:
        // Wait until the NTRIP client is enabled
        if (r4aNtripClientEnable)
        {
            // Don't allow the client to restart if a forced shutdown occured
            if (r4aNtripClientForcedShutdown)
                errorPrint->println("ERROR: Please clear the forced error before starting the NTRIP client!");
            else if (!r4aNtripClientCasterHost)
                errorPrint->println("ERROR: Please set the NTRIP caster host name!");
            else if (!r4aNtripClientCasterMountPoint)
                errorPrint->println("ERROR: Please set the NTRIP caster mount point!");
            else if (!r4aNtripClientCasterUser)
                errorPrint->println("ERROR: Please set the NTRIP caster user name!");
            else
            {
                // NTRIP client is enabled with all the necessary paramters
                r4aNtripClientStart(display);
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
            r4aNtripClient._client = new NetworkClient();
            if (!r4aNtripClient._client)
            {
                // Failed to allocate the _client structure
                errorPrint->println("ERROR: Failed to allocate the _client structure!");
                r4aNtripClientForceShutdown(display);
            }
            else
            {
                // Account for this connection attempt
                r4aNtripClient._connectionAttempts += 1;
                r4aNtripClient._connectionAttemptsTotal += 1;

                // The network is available for the NTRIP client
                r4aNtripClientSetState(NTRIP_CLIENT_CONNECTING, display);
            }
        }
        break;

    case NTRIP_CLIENT_CONNECTING:
        // Delay before opening the NTRIP client connection
        if ((millis() - r4aNtripClient._timer) >= r4aNtripClient._connectionDelayMsec)
        {
            // Open connection to NTRIP caster service
            if (!r4aNtripClientConnect(debugState))
            {
                // Assume service not available
                if (r4aNtripClientConnectLimitReached(display))
                    errorPrint->println(
                        "NTRIP caster failed to connect. Do you have your caster address and port correct?");
            }
            else
            {
                // Socket opened to NTRIP system
                if (debugState)
                    debugState->printf("NTRIP Client waiting for response from %s:%d\r\n",
                                       r4aNtripClientCasterHost, r4aNtripClientCasterPort);

                // Start the response timer
                r4aNtripClient._timer = millis();
                r4aNtripClient._responseLength = 0;
                r4aNtripClientSetState(NTRIP_CLIENT_WAIT_RESPONSE, display);
            }
        }
        break;

    case NTRIP_CLIENT_WAIT_RESPONSE:
        // At least a few bytes received, wait ultil the response is done
        // Save the initial portion of the response
        length = r4aNtripClient._client->available();
        if (length)
        {
            // Check for the end of the response
            if (r4aNtripClient._client->peek() == 0xd3)
                // End of response
                r4aNtripClientSetState(NTRIP_CLIENT_HANDLE_RESPONSE, display);

            // Get the next portion of the response
            else
                r4aNtripClientResponse(display, length);
        }

        // Check for the end of the response timeout
        else if ((millis() - r4aNtripClient._timer) >= r4aNtripClientResponseDone)
            // End of response
            r4aNtripClientSetState(NTRIP_CLIENT_HANDLE_RESPONSE, display);

        // Check for a response timeout
        else if ((!r4aNtripClient._responseLength)
            && ((millis() - r4aNtripClient._timer) > r4aNtripClientResponseTimeout))
        {
            // NTRIP web service did not respond
           if (r4aNtripClientConnectLimitReached(display))
                errorPrint->println("NTRIP Caster failed to respond. Do you have your caster address and port correct?");
        }
        break;

    case NTRIP_CLIENT_HANDLE_RESPONSE:
        // Process the respones
        response = r4aNtripClient._responseBuffer;

        // Separate the output
        if (debugState)
            debugState->println();

        // Look for various responses
        if (strstr(response, "200") != nullptr) //'200' found
        {
            // Check for banned from the NTRIP caster
            if (strcasestr(response, "banned") != nullptr)
            {
                errorPrint->printf("NTRIP Client banned from %s!\r\n",
                                   r4aNtripClientCasterHost);

                // No longer allowed to access this website
                // Try again later
                r4aNtripClientForceShutdown(display);
            }

            // Check for using sandbox
            else if (strcasestr(response, "sandbox") != nullptr)
            {
                errorPrint->printf("NTRIP Client redirected to sandbox on %s!\r\n",
                                   r4aNtripClientCasterHost);

                // Re-attempted after a period of time. Shuts down
                // NTRIP Client if limit reached.
                r4aNtripClientConnectLimitReached(display);
            }

            // Check for failed lookup
            else if (strcasestr(response, "SOURCETABLE") != nullptr)
            {
                errorPrint->printf("Mount point %s not found on %s!\r\n",
                                   r4aNtripClientCasterMountPoint, r4aNtripClientCasterHost);

                // Stop NTRIP client operations
                r4aNtripClientForceShutdown(display);
            }
            else
            {
                // Timeout receiving NTRIP data, retry the NTRIP client connection
                if (r4aNtpOnline)
                    errorPrint->printf("NTRIP Client connected to %s:%d/%s at %s\r\n",
                                       r4aNtripClientCasterHost, r4aNtripClientCasterPort,
                                       r4aNtripClientCasterMountPoint,
                                       r4aNtpGetTime24(r4aNtpGetEpochTime()).c_str());
                else
                    errorPrint->printf("NTRIP Client connected to %s:%d\r\n",
                                       r4aNtripClientCasterHost, r4aNtripClientCasterPort);

                // Connection is now open, start the NTRIP receive data timer
                r4aNtripClient._startTime = millis();
                r4aNtripClient._timer = r4aNtripClient._startTime;

                // Start processing correction data
                r4aNtripClientSetState(NTRIP_CLIENT_CONNECTED, display);
            }
        }

        // Check for unauthorized user
        else if (strstr(response, "401") != nullptr)
        {
            // Look for '401 Unauthorized'
            errorPrint->printf("User %s not authorized on NTRIP Caster %s!\r\n"
                               "Are you sure your caster credentials are correct?\r\n",
                               r4aNtripClientCasterUser, r4aNtripClientCasterHost);

            // Stop NTRIP client operations
           r4aNtripClientForceShutdown(display);
        }

        // Check for startup phase
        else if (strstr(response, "406") != nullptr)
        {
            // Look for '406 In Start Up Phase'
            errorPrint->printf("NTRIP caster %s is in its startup phase!\r\n",
                               r4aNtripClientCasterHost);

            // Stop NTRIP client operations
            r4aNtripClientRestart(display);
        }

        // Other errors returned by the caster
        else
        {
            if (!r4aNtripClientEnable)
                errorPrint->print(r4aNtripClient._responseBuffer);
            if (r4aNtripClient._responseLength)
            {
                errorPrint->printf("NTRIP caster %s responded with an error!\r\n",
                                   r4aNtripClientCasterHost);

                // Stop NTRIP client operations
                r4aNtripClientForceShutdown(display);
            }
            else
            {
                errorPrint->printf("Response timeout from NTRIP caster %s!\r\n",
                                   r4aNtripClientCasterHost);

                // Re-attempted after a period of time. Shuts down
                // NTRIP Client if limit reached.
                r4aNtripClientConnectLimitReached(display);
            }
        }
        break;

    case NTRIP_CLIENT_CONNECTED:
        // Check for a broken connection
        if (!r4aNtripClient._client->connected())
        {
            // Broken connection, retry the NTRIP client connection
            errorPrint->println("NTRIP Client connection to caster was broken");
            r4aNtripClientRestart(display);
        }
        else
        {
            // Handle other types of NTRIP connection failures to prevent
            // hammering the NTRIP caster with rapid connection attempts.
            // A fast reconnect is reasonable after a long NTRIP caster
            // connection.  However increasing backoff delays should be
            // added when the NTRIP caster fails after a short connection
            // interval.
            if (r4aNtripClient._connectionAttempts
                && ((millis() - r4aNtripClient._startTime) > R4A_NTRIP_CLIENT_CONNECTION_TIME))
            {
                // After a long connection period, reset the attempt counter
                r4aNtripClient._connectionAttempts = 0;
                if (debugState)
                    debugState->println("NTRIP Client resetting connection attempt counter and timeout");
            }

            // Check for timeout receiving NTRIP data
            if (r4aNtripClient._client->available() == 0)
            {
                // Don't fail during retransmission attempts
                if ((millis() - r4aNtripClient._timer) > r4aNtripClientReceiveTimeout)
                {
                    // Timeout receiving NTRIP data, retry the NTRIP client connection
                    if (r4aNtpOnline)
                        errorPrint->printf("NTRIP Client timeout receiving data at %s\r\n",
                                           r4aNtpGetTime24(r4aNtpGetEpochTime()).c_str());
                    else
                        errorPrint->println("NTRIP Client timeout receiving data");
                    r4aNtripClientRestart(display);
                }
            }
            else
            {
                int availableBytes;

                // Receive data from the NTRIP Caster
                availableBytes = r4aNtripClient._client->available();
                while (availableBytes)
                {
                    r4aNtripClientRbAddData(availableBytes, display);
                    availableBytes = r4aNtripClient._client->available();
                }
            }
        }
        break;
    }
}

//*********************************************************************
// Verify the NTRIP client tables
void r4aNtripClientValidateTables()
{
    if (r4aNtripClientStateNameEntries != NTRIP_CLIENT_STATE_MAX)
        r4aReportFatalError("Fix r4aNtripClientStateNameEntries to match _state");
}
