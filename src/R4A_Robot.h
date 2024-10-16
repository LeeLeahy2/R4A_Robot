/**********************************************************************
  R4A_Robot.h

  Robots-For-All (R4A)
  Declare the robot support
**********************************************************************/

#ifndef __R4A_ROBOT_H__
#define __R4A_ROBOT_H__

#include <Arduino.h>            // Built-in
#include <base64.h>             // Built-in, needed for NTRIP Client credential encoding
#include <esp32-hal-spi.h>      // Built-in
#include <WiFi.h>               // Built-in
#include <WiFiServer.h>         // Built-in

// External libraries
#include <NTPClient.h>          // https://github.com/arduino-libraries/NTPClient
#include <TimeLib.h>            // In Time library, format and parse time values

//****************************************
// Constants
//****************************************

// Define distance constants
#define R4A_MILLIMETERS_PER_METER       ((double)1000.)
#define R4A_METERS_PER_KILOMETER        ((double)1000.)
#define R4A_MILLIMETERS_PER_KILOMETER   (MILLIMETERS_PER_METER * METERS_PER_KILOMETER)

#define R4A_INCHES_PER_FOOT             ((double)12.)
#define R4A_FEET_PER_MILE               ((double)5280.)

#define R4A_MILLIMETERS_PER_INCH        ((double)25.4)
#define R4A_MILLIMETERS_PER_FOOT        ((double)(R4A_MILLIMETERS_PER_INCH * R4A_INCHES_PER_FOOT))

// Define time constants
#define R4A_MILLISECONDS_IN_A_SECOND    1000
#define R4A_SECONDS_IN_A_MINUTE         60
#define R4A_MILLISECONDS_IN_A_MINUTE    (R4A_SECONDS_IN_A_MINUTE * R4A_MILLISECONDS_IN_A_SECOND)
#define R4A_MINUTES_IN_AN_HOUR          60
#define R4A_MILLISECONDS_IN_AN_HOUR     (R4A_MINUTES_IN_AN_HOUR * R4A_MILLISECONDS_IN_A_MINUTE)
#define R4A_HOURS_IN_A_DAY              24
#define R4A_MILLISECONDS_IN_A_DAY       (R4A_HOURS_IN_A_DAY * R4A_MILLISECONDS_IN_AN_HOUR)

#define R4A_SECONDS_IN_AN_HOUR          (R4A_MINUTES_IN_AN_HOUR * R4A_SECONDS_IN_A_MINUTE)
#define R4A_SECONDS_IN_A_DAY            (R4A_HOURS_IN_A_DAY * R4A_SECONDS_IN_AN_HOUR)

#define R4A_MINUTES_IN_A_DAY            (R4A_HOURS_IN_A_DAY * R4A_MINUTES_IN_AN_HOUR)

//****************************************
// Forward Declarations
//****************************************

class R4A_TELNET_SERVER;

//****************************************
// Command Processor API
//****************************************

// Process the line received via telnet
// Inputs:
//   command: Zero terminated array of characters
//   display: Address of Print object for output
// Outputs:
//   Returns true if the telnet connection should be broken and false otherwise
typedef bool (* R4A_COMMAND_PROCESSOR)(const char * command,
                                       Print * display);

// Support sub-menu processing by changing this value
extern volatile R4A_COMMAND_PROCESSOR r4aProcessCommand;

//****************************************
// Dump Buffer API
//****************************************

// Display a buffer contents in hexadecimal and ASCII
// Inputs:
//   offset: Offset of the first byte in the buffer, 0 or buffer address
//   buffer: Address of the buffer containing the data
//   length: Length of the buffer in bytes
//   display: Device used for output
void r4aDumpBuffer(uint32_t offset,
                   const uint8_t *buffer,
                   uint32_t length,
                   Print * display = &Serial);

//****************************************
// LED API
//****************************************

#define R4A_LED_AQUA                    0x0000ffff
#define R4A_LED_BLACK                   0x00000000
#define R4A_LED_BLUE                    0x000000ff
#define R4A_LED_CYAN                    0x00ff00ff
#define R4A_LED_GREEN                   0x0000ff00
#define R4A_LED_OFF                     R4A_LED_BLACK
#define R4A_LED_ORANGE                  0x00ff8000
#define R4A_LED_PINK                    R4A_LED_CYAN
#define R4A_LED_PURPLE                  0x008000ff
#define R4A_LED_RED                     0x00ff0000
#define R4A_LED_WHITE_ALL               0xffffffff
#define R4A_LED_WHITE_RGB               0x00ffffff
#define R4A_LED_WHITE_RGBW              0xff000000
#define R4A_LED_YELLOW                  0x00ffff00

// Set the WS2812 LED colors
// Inputs:
//   ledNumber: Index into the LED color array
//   color: Red bits: 23 - 16, Green bits: 15 - 8, Blue bits; 7 - 0
void r4aLEDSetColorRgb(uint8_t ledNumber, uint32_t color);

// Set the WS2812 LED colors
// Inputs:
//   ledNumber: Index into the LED color array
//   red: Intensity of the red LED
//   green: Intensity of the green LED
//   blue: Intensity of the blue LED
void r4aLEDSetColorRgb(uint8_t ledNumber,
                       uint8_t red,
                       uint8_t green,
                       uint8_t blue);

// Set the SK6812RGBW LED colors
// Inputs:
//   ledNumber: Index into the LED color array
//   color: White bits: 31 - 24, Red bits: 23 - 16, Green bits: 15 - 8, Blue bits; 7 - 0
void r4aLEDSetColorWrgb(uint8_t ledNumber,
                        uint32_t color);

// Set the SK6812RGBW LED colors
// Inputs:
//   ledNumber: Index into the LED color array
//   white: Intensity of the white LED
//   red: Intensity of the red LED
//   green: Intensity of the green LED
//   blue: Intensity of the blue LED
void r4aLEDSetColorWrgb(uint8_t ledNumber,
                        uint8_t white,
                        uint8_t red,
                        uint8_t green,
                        uint8_t blue);

// Set the intensity
// Inputs:
//   Intensity: A number in the range of (0 - 255), 0 = off, 255 = on full
void r4aLEDSetIntensity(uint8_t intensity);

// Initialize the WS2812 LEDs
// Inputs:
//   spiNumber: Number of the SPI bus
//   pinMOSI: Pin number of the MOSI pin that connects to the SPI TX data line
//   ClockHz: SPI clock frequency in Hertz
//   numberOfLEDs: Number of multi-color LEDs in the string
// Outputs:
//   Returns true for successful initialization and false upon error
bool r4aLEDSetup(uint8_t spiNumber,
                 uint8_t pinMOSI,
                 uint32_t clockHz,
                 uint8_t numberOfLEDs);

// Turn off the LEDs
void r4aLEDsOff();

// Update the colors on the WS2812 LEDs
// Inputs:
//   updateRequest: When true causes color data to be sent to the LEDS
//                  When false only updates LEDs if color or intensity was changed
void r4aLEDUpdate(bool updateRequest);

//****************************************
// Lock API
//****************************************

// Take out a lock
// Inputs:
//   lock: Address of the lock
void r4aLockAcquire(volatile int * lock);

// Release a lock
// Inputs:
//   lock: Address of the lock
void r4aLockRelease(volatile int * lock);

//****************************************
// Menu API
//****************************************

#define R4A_MENU_NONE           0
#define R4A_MENU_MAIN           1

// Forward declarations
typedef struct _R4A_MENU_ENTRY R4A_MENU_ENTRY;

typedef void (*R4A_PRE_MENU_DISPLAY)(Print * display);

typedef struct _R4A_MENU_TABLE
{
    const char * menuName;          // Name of the menu
    R4A_PRE_MENU_DISPLAY preMenu;   // Routine to display data before the menu
    const R4A_MENU_ENTRY * firstEntry; // First entry in the menu
    int32_t menuEntryCount;         // Number of entries in the table
} R4A_MENU_TABLE;

// Process a menu item
// Inputs:
//   menuEntry: Address of the menu entry associated with the command
//   command: Full command line
//   display: Address of the Print object for output
typedef void (*R4A_MENU_ROUTINE)(const struct _R4A_MENU_ENTRY * menuEntry,
                                 const char * command,
                                 Print * display);

// Display help for a menu item
// Inputs:
//   menuEntry: Address of the menu entry to display help
//   align: Zero terminated string of spaces to align the help display
//   display: Address of the Print object for output
typedef void (*R4A_HELP_ROUTINE)(const struct _R4A_MENU_ENTRY * menuEntry,
                                 const char * align,
                                 Print * display);

typedef struct _R4A_MENU_ENTRY
{
    const char * command;           // Command: Expected serial input
    R4A_MENU_ROUTINE menuRoutine;   // Routine to process the menu
    intptr_t menuParameter;         // Parameter for the menu routine
    R4A_HELP_ROUTINE helpRoutine;   // Routine to display the help message
    int align;                      // Command length adjustment for alignment
    const char * helpText;          // Help text to display
} R4A_MENU_ENTRY;

class R4A_MENU
{
  private:

    const R4A_MENU_TABLE * _menu;            // Current menu to display and use
    const R4A_MENU_TABLE * const _menuTable; // Address of all menu descriptions
    const int _menuTableEntries;             // Number of entries in the menu table

  public:

    bool _blankLineBeforePreMenu;       // Display a blank line before the preMenu
    bool _blankLineBeforeMenuHeader;    // Display a blank line before the menu header
    bool _blankLineAfterMenuHeader;     // Display a blank line after the menu header
    bool _alignCommands;                // Align the commands
    bool _blankLineAfterMenu;           // Display a blank line after the menu

    // Constructor
    // Inputs:
    //   menuTable: Address of table containing the menu descriptions, the
    //              main menu must be the first entry in the table.
    //   menuEntries: Number of entries in the menu table
    //   blankLineBeforePreMenu: Display a blank line before the preMenu
    //   blankLineBeforeMenuHeader: Display a blank line before the menu header
    //   blankLineAfterMenuHeader: Display a blank line after the menu header
    //   alignCommands: Align the commands
    //   blankLineAfterMenu: Display a blank line after the menu
    R4A_MENU(const R4A_MENU_TABLE * menuTable,
            int menuEntries,
            bool blankLineBeforePreMenu = true,
            bool blankLineBeforeMenuHeader = true,
            bool blankLineAfterMenuHeader = false,
            bool alignCommands = true,
            bool blankLineAfterMenu = false)
        : _menuTable{menuTable}, _menuTableEntries{menuEntries},
          _blankLineBeforePreMenu{blankLineBeforePreMenu},
          _blankLineBeforeMenuHeader{blankLineBeforeMenuHeader},
          _blankLineAfterMenuHeader{blankLineAfterMenuHeader},
          _alignCommands{alignCommands}, _blankLineAfterMenu{blankLineAfterMenu}
    {
    }

    // Display the menu object contents
    // Inputs:
    //   display: Address of the Print object for output
    void display(Print * display);

    // Process a menu command when specified or display the menu when command
    // is nullptr.
    // Inputs:
    //   menuTable: Address of the address of the menuTable (description of the menu)
    //   command: Command string
    //   display: Address of the Print object for output
    // Outputs:
    //   True when exiting the menu system, false if still in the menu system
    bool process(const char * command,
                 Print * display = &Serial);
};

// Display the boolean as enabled or disabled
// Inputs:
//   menuEntry: Address of the menu entry to display
//   align: Zero terminated string of spaces to align the help display
//   display: Address of the Print object for output
void r4aMenuBoolHelp(const R4A_MENU_ENTRY * menuEntry,
                     const char * align,
                     Print * display);

// Toggle boolean value
// Inputs:
//   menuEntry: Address of the menu entry containing the address to toggle
//   display: Address of the Print object for output
void r4aMenuBoolToggle(const R4A_MENU_ENTRY * menuEntry,
                       const char * command,
                       Print * display);

//****************************************
// LED Menu API
//****************************************

extern const R4A_MENU_ENTRY r4aLEDMenuTable[];
#define R4A_LED_MENU_ENTRIES    5

// Set the WS2812 LED color
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aLEDMenuColor3(const R4A_MENU_ENTRY * menuEntry,
                      const char * command,
                      Print * display);

// Set the SK6812RGBW LED color
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aLEDMenuColor4(const R4A_MENU_ENTRY * menuEntry,
                      const char * command,
                      Print * display);

// Display the LED status
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aLEDMenuDisplay(const R4A_MENU_ENTRY * menuEntry,
                       const char * command,
                       Print * display);

// Display the help text with iii
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   align: Zero terminated string of spaces for alignment
//   display: Device used for output
void r4aLEDMenuHelpiii(const struct _R4A_MENU_ENTRY * menuEntry,
                       const char * align,
                       Print * display);

// Display the help text with ll and cccccccc
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   align: Zero terminated string of spaces for alignment
//   display: Device used for output
void r4aLEDMenuHelpllcccc(const struct _R4A_MENU_ENTRY * menuEntry,
                          const char * align,
                          Print * display);

// Set the LED intensity
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aLEDMenuIntensity(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

// Turn off the LEDs
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aLEDMenuOff(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

//****************************************
// NTP API
//****************************************

extern bool r4aNtpDebugStates; // Set true to display state changes

// Display the date and time
// Inputs:
//   display: Device used for output
void r4aNtpDisplayDateTime(Print * display = &Serial);

// Get the date string
// Inputs:
//   seconds: The number of seconds from 1 Jan 1970
// Outputs:
//   Returns the date as yyyy-mm-dd or "Time not set"
String r4aNtpGetDate(uint32_t seconds);

// Get the number of seconds from 1 Jan 1970
// Outputs:
//   Returns the number of seconds from 1 Jan 1970
uint32_t r4aNtpGetEpochTime();

// Get the time as hh:mm:ss
// Outputs:
//   Returns time as hh:mm:ss or "Time not set"
String r4aNtpGetTime();

// Get the time string in 12 hour format
// Outputs:
//   Returns time as hh:mm:ss xM or "Time not set"
String r4aNtpGetTime12(uint32_t seconds);

// Get the time string in 24 hour format
// Outputs:
//   Returns time as hh:mm:ss or "Time not set"
String r4aNtpGetTime24(uint32_t seconds);

// Determine if the time is valid
// Outputs:
//   Returns true when the time and date are valid
bool r4aNtpIsTimeValid();

// Initialize the NTP server
// Inputs:
//   displayInitialTime: Display the time once NTP gets the time
void r4aNtpSetup(bool displayInitialTime);

// Initialize the NTP server
// Inputs:
//   timeZoneOffsetSeconds: Time zone offset from GMT in seconds
//   displayInitialTime: Display the time once NTP gets the time
void r4aNtpSetup(long timeZoneOffsetSeconds, bool displayInitialTime);

// Update the NTP client and system time
// Inputs:
//   wifiConnected: True when WiFi is connected to an access point, false otherwise
void r4aNtpUpdate(bool wifiConnected);

//****************************************
// NTRIP Client
//****************************************

// Size of the credentials buffer in bytes
#define R4A_CREDENTIALS_BUFFER_SIZE             512

// Size of the response buffer
#define R4A_NTRIP_CLIENT_RESPONSE_BUFFER_SIZE   512

#define R4A_NTRIP_CLIENT_RING_BUFFER_BYTES      8192 // Size of the ring buffer

// Most incoming data is around 500 bytes but may be larger
#define R4A_RTCM_DATA_SIZE                      (512 * 4)

// NTRIP client server request buffer size
#define R4A_SERVER_BUFFER_SIZE                  (CREDENTIALS_BUFFER_SIZE + 3)

// NTRIP client connection delay before resetting the connect accempt counter
#define R4A_NTRIP_CLIENT_CONNECTION_TIME        (5 * R4A_MILLISECONDS_IN_A_MINUTE)

// Define the back-off intervals between connection attempts in milliseconds
extern const uint32_t r4aNtripClientBbackoffIntervalMsec[];
extern const int r4aNtripClientBbackoffCount;

const char *const r4aNtripClientStateName[] = {"NTRIP_CLIENT_OFF",
                                            "NTRIP_CLIENT_WAIT_FOR_WIFI",
                                            "NTRIP_CLIENT_CONNECTING",
                                            "NTRIP_CLIENT_WAIT_RESPONSE",
                                            "NTRIP_CLIENT_HANDLE_RESPONSE",
                                            "NTRIP_CLIENT_CONNECTED"};
const int r4aNtripClientStateNameEntries = sizeof(r4aNtripClientStateName) / sizeof(r4aNtripClientStateName[0]);


//----------------------------------------
// Parameters
//----------------------------------------

extern const char * r4aNtripClientCasterHost;       // NTRIP caster host name
extern const char * r4aNtripClientCasterMountPoint; // NTRIP caster mount point
extern uint16_t r4aNtripClientCasterPort;           // Typically 2101
extern const char * r4aNtripClientCasterUser;       // e-mail address
extern const char * r4aNtripClientCasterUserPW;

extern const char * r4aNtripClientCompany;      // "Company" for your robot

extern volatile bool r4aNtripClientDebugRtcm;   // Display RTCM data
extern volatile bool r4aNtripClientDebugState;  // Display NTRIP client state transitions

extern volatile bool r4aNtripClientEnable;      // True enables the NTRIP client
extern volatile bool r4aNtripClientForcedShutdown;  // Set true during fatal NTRIP errors

extern const char * r4aNtripClientProduct;          // "Product" for your robot
extern const char * r4aNtripClientProductVersion;   // "Version" of your robot

extern uint32_t r4aNtripClientReceiveTimeout;   // mSec timeout for beginning of HTTP response from NTRIP caster
extern uint32_t r4aNtripClientResponseDone;     // mSec timeout waiting for end of HTTP response from NTRIP caster
extern uint32_t r4aNtripClientResponseTimeout;  // mSec timeout for RTCM data from NTRIP caster

class R4A_NTRIP_CLIENT
{
  private:

    //----------------------------------------
    // Constants
    //----------------------------------------

    const int _minimumRxBytes = 32;       // Minimum number of bytes to process

    // Define the NTRIP client states
    enum NTRIPClientState
    {
        NTRIP_CLIENT_OFF = 0,           // NTRIP client disabled or missing parameters
        NTRIP_CLIENT_WAIT_FOR_WIFI,     // Connecting to WiFi access point
        NTRIP_CLIENT_CONNECTING,        // Attempting a connection to the NTRIP caster
        NTRIP_CLIENT_WAIT_RESPONSE,     // Wait for a response from the NTRIP caster
        NTRIP_CLIENT_HANDLE_RESPONSE,   // Process the response
        NTRIP_CLIENT_CONNECTED,         // Connected to the NTRIP caster
        // Insert new states here
        NTRIP_CLIENT_STATE_MAX // Last entry in the state list
    };

    //----------------------------------------
    // Locals
    //----------------------------------------

    // The network connection to the NTRIP caster to obtain RTCM data.
    WiFiClient * _client;
    volatile uint8_t _state;

    // Throttle the time between connection attempts
    uint32_t _connectionDelayMsec; // Delay before making the connection
    int _connectionAttempts; // Count the number of connection attempts between restarts
    int _connectionAttemptsTotal; // Count the number of connection attempts absolutely
    bool _forcedShutdown;

    // NTRIP client timer usage:
    //  * Reconnection delay
    //  * Measure the connection response time
    //  * Receive NTRIP data timeout
    uint32_t _timer;
    uint32_t _startTime; // For calculating uptime

    char _responseBuffer[R4A_NTRIP_CLIENT_RESPONSE_BUFFER_SIZE];
    int _responseLength;

    // Ring buffer for NTRIP correction data
    volatile int _rbHead;  // RX data offset in ring buffer to add data
    volatile int _rbTail;  // GNSS offset in ring buffer to remove data
    uint8_t _ringBuffer[R4A_NTRIP_CLIENT_RING_BUFFER_BYTES];
    uint8_t _i2cTransactionSize;

  public:

    // Constructor
    R4A_NTRIP_CLIENT()
        : _client{nullptr},
          _state{NTRIP_CLIENT_OFF},
          _connectionDelayMsec{r4aNtripClientBbackoffIntervalMsec[0]},
          _connectionAttempts{0},
          _connectionAttemptsTotal{0},
          _forcedShutdown{false},
          _timer{0},
          _startTime{0},
          _responseLength{0},
          _rbHead{0},
          _rbTail{0},
          _i2cTransactionSize{0}
    {
    };

    // Attempt to connect to the NTRIP caster
    bool connect();

    // Determine if another connection is possible or if the limit has been reached
    bool connectLimitReached();

    void displayResponse(Print * display, char * response, int bytesRead);

    // NTRIP Client was turned off due to an error. Don't allow restart!
    void forceShutdown();

    // Get the device for debug output
    virtual Print * getSerial();

    // Get the I2C bus transaction size
    virtual uint8_t i2cTransactionSize();

    // Print the NTRIP client state summary
    void printStateSummary(Print * display);

    // Print the NTRIP Client status
    void printStatus(Print * display);

    // Send data to the GNSS radio
    virtual int pushRawData(uint8_t * buffer, int bytesToPush, Print * display);

    // Add data to the ring buffer
    int rbAddData(int length);

    // Remove data from the ring buffer
    int rbRemoveData(Print * display);

    // Read the response from the NTRIP client
    void response(Print *display, int length);

    // Restart the NTRIP client
    void restart();

    // Update the state of the NTRIP client state machine
    void setState(uint8_t newState);

    // Start the NTRIP client
    void start();

    // Shutdown or restart the NTRIP client
    bool stop(bool shutdown);

    // Check for the arrival of any correction data. Push it to the GNSS.
    // Stop task if the connection has dropped or if we receive no data for
    // _receeiveTimeout
    void update(bool wifiConnected);

    // Verify the NTRIP client tables
    void validateTables();
};

//****************************************
// NTRIP Client Menu API
//****************************************

extern const R4A_MENU_ENTRY r4aNtripClientMenuTable[];
#define R4A_NTRIP_CLIENT_MENU_ENTRIES   5   // NTRIP client menu table entries

//****************************************
// ReadLine API
//****************************************

// Read a line of input from a serial port into a character array
// Inputs:
//   port: Address of a HardwareSerial port structure
//   echo: Specify true to enable echo of input characters and false otherwise
//   buffer: Address of a string that contains the input line
// Outputs:
//   nullptr when the line is not complete
String * r4aReadLine(bool echo, String * buffer, HardwareSerial * port = &Serial);

// Read a line of input from a WiFi client into a character array
// Inputs:
//   port: Address of a WiFiClient port structure
//   echo: Specify true to enable echo of input characters and false otherwise
//   buffer: Address of a string that contains the input line
// Outputs:
//   nullptr when the line is not complete
String * r4aReadLine(bool echo, String * buffer, WiFiClient * port);

//****************************************
// Robot Challenge class
//****************************************

class R4A_ROBOT_CHALLENGE
{
  public:

    const char * _name; // Name of the challenge
    const uint32_t _duration; // Number of seconds to run the robot challenge

    // Constructor
    R4A_ROBOT_CHALLENGE(const char * name, uint32_t durationSec)
        : _name{name}, _duration{durationSec}
    {
    }

    // The robotRunning routine calls this routine to actually perform
    // the challenge.  This routine typically reads a sensor and may
    // optionally adjust the motors based upon the sensor reading.  The
    // routine then must return.  The robot layer will call this routine
    // multiple times during the robot operation.
    virtual void challenge();

    // The robotStart calls this routine before switching to the initial
    // delay state.
    virtual void init();

    // The initial delay routine calls this routine just before calling
    // the challenge routine for the first time.
    virtual void start();

    // The robotStop routine calls this routine to stop the motors and
    // perform any other actions.
    virtual void stop();
};

//****************************************
// Robot class
//****************************************

class R4A_ROBOT;

// Display the delta time
// Inputs:
//   deltaMsec: Milliseconds to display
typedef void (* R4A_ROBOT_TIME_CALLBACK)(uint32_t deltaMsec);

class R4A_ROBOT
{
  private:

    volatile bool _busy;    // True while challenge is running
    volatile R4A_ROBOT_CHALLENGE * _challenge;  // Address of challenge object
    const int _core;        // CPU core number
    const uint32_t _afterRunMsec; // Delay after robot's run and switching to idle
    uint32_t _endMsec;      // Challenge end time in milliseconds since boot
    uint32_t _idleMsec;     // Last idle time in milliseconds since boot
    uint32_t _initMsec;     // Challenge init time in milliseconds since boot
    uint32_t _nextDisplayMsec;  // Next time display time should be called in milliseconds since boot
    const uint32_t _startDelayMsec; // Number of milliseconds before starting the challenge
    uint32_t _startMsec;    // Challenge start time in milliseconds since boot
    uint32_t _stopMsec;     // Challenge stop time in milliseconds since boot
    uint8_t _state;         // Next state for robot operation

    enum ROBOT_STATES
    {
        STATE_IDLE = 0,     // The robot layer is idle
        STATE_COUNT_DOWN,   // The robot layer is counting down to start
        STATE_RUNNING,      // The robot layer is running the challenge
        STATE_STOP,         // The robot layer is stopped
    };

    // Called by the init routine to display the countdown time
    // Called by the initial delay routine to display the countdown time
    // Called by the stop routine to display the actual challenge duration
    // Inputs:
    //   mSecToStart: Milliseconds until the robot is to start the
    //                challenge
    R4A_ROBOT_TIME_CALLBACK _displayTime;   // May be set to nullptr

    // Called by the update routine when the robot is not running a challenge
    // Inputs:
    //   mSecToStart: Milliseconds until the robot is to start the
    //                challenge
    R4A_ROBOT_TIME_CALLBACK _idle;  // Maybe set to nullptr

    // Perform the initial delay
    // Inputs:
    //   currentMsec: Milliseconds since boot
    void initialDelay(uint32_t currentMsec);

    // Run the robot challenge
    // Inputs:
    //   currentMsec: Milliseconds since boot
    void running(uint32_t currentMsec);

    // Perform activity while the robot is stopped
    // Inputs:
    //   currentMsec: Milliseconds since boot
    void stopped(uint32_t currentMsec);

  public:

    // Constructor
    // Inputs:
    //   core: CPU core that is running the robot layer
    //   startDelaySec: Number of seconds before the robot starts a challenge
    //   afterRunSec: Number of seconds after the robot completes a challenge
    //                before the robot layer switches back to the idle state
    //   idle: Address of the idle routine, may be nullptr
    //   displayTime: Address of the routine to display the time, may be nullptr
    R4A_ROBOT(int core = 0,
              uint32_t startDelaySec = 10,
              uint32_t afterRunSec = 30,
              R4A_ROBOT_TIME_CALLBACK idle = nullptr,
              R4A_ROBOT_TIME_CALLBACK displayTime = nullptr)
        : _afterRunMsec{afterRunSec * R4A_MILLISECONDS_IN_A_SECOND},
          _busy{false},
          _challenge{nullptr},
          _core{core},
          _displayTime{displayTime},
          _idle{idle},
          _startDelayMsec{startDelaySec * R4A_MILLISECONDS_IN_A_SECOND},
          _state{STATE_IDLE}
    {
    }

    // Determine if it is possible to start the robot
    // Inputs:
    //   challenge: Address of challenge object
    //   duration: Number of seconds to run the challenge
    //   display: Device used for output
    bool init(R4A_ROBOT_CHALLENGE * challenge,
              uint32_t duration,
              Print * display = &Serial);

    // Determine if the robot layer is active
    // Outputs:
    //   Returns true when the challenge is running and false otherwise
    bool isActive()
    {
        return ((_state == STATE_COUNT_DOWN) || (_state == STATE_RUNNING));
    }

    // Stop the robot
    // Inputs:
    //   currentMsec: Milliseconds since boot
    //   display: Device used for output
    void stop(uint32_t currentMsec, Print * display = &Serial);

    // Update the robot state
    // Inputs:
    //   currentMsec: Milliseconds since boot
    void update(uint32_t currentMsec);
};

//****************************************
// Serial API
//****************************************

// Repeatedly display a fatal error message
// Inputs:
//   errorMessage: Zero terminated string of characters containing the
//                 error mesage to be displayed
//   display: Device used for output
void r4aReportFatalError(const char * errorMessage,
                         Print * display = &Serial);

// Process serial menu item
// Inputs:
//   menu: Address of the menu object
void r4aSerialMenu(R4A_MENU * menu);

//****************************************
// SPI API
//****************************************

class R4A_SPI
{
  public:

    // Allocate DMA buffer
    // Inputs:
    //   length: Number of data bytes to allocate
    // Outputs:
    //   Returns the buffer address if successful and nullptr otherwise
    virtual uint8_t * allocateDmaBuffer(int length);

    // Initialize the SPI controller
    // Inputs:
    //   spiNumber: Number of the SPI controller
    //   pinMOSI: SPI TX data pin number
    //   clockHz: SPI clock frequency in Hertz
    // Outputs:
    //   Return true if successful and false upon failure
    virtual bool begin(uint8_t spiNumber, uint8_t pinMOSI, uint32_t clockHz);

    // Transfer data to the SPI device
    // Inputs:
    //   txBuffer: Address of the buffer containing the data to send
    //   rxBuffer: Address of the receive data buffer
    //   length: Number of data bytes to transfer
    virtual void transfer(const uint8_t * txBuffer,
                          uint8_t * rxBuffer,
                          uint32_t length);
};

extern R4A_SPI * r4aSpi;

//****************************************
// Stricmp API
//****************************************

// Compare two strings ignoring case
// Inputs:
//   str1: Address of a zero terminated string of characters
//   str2: Address of a zero terminated string of characters
// Outputs:
//   Returns the delta value of the last comparison (str1[x] - str2[x])
int r4aStricmp(const char *str1, const char *str2);

//****************************************
// Strincmp API
//****************************************

// Compare two strings limited to N characters ignoring case
// Inputs:
//   str1: Address of a zero terminated string of characters
//   str2: Address of a zero terminated string of characters
//   length: Maximum number of characters to compare
// Outputs:
//   Returns the delta value of the last comparison (str1[x] - str2[x])
int r4aStrincmp(const char *str1, const char *str2, int length);

//****************************************
// Telnet Client API
//****************************************

// Forward declaration
class R4A_TELNET_SERVER;

//*********************************************************************
// Telnet client
class R4A_TELNET_CLIENT : public WiFiClient
{
private:
    R4A_TELNET_CLIENT * _nextClient; // Next client in the server's client list
    String _command; // User command received via telnet
    R4A_MENU _menu;  // Strcture describing the menu system

    // Allow the server to access the command string
    friend R4A_TELNET_SERVER;

public:

    // Constructor
    // Inputs:
    //   menuTable: Address of table containing the menu descriptions, the
    //              main menu must be the first entry in the table.
    //   menuEntries: Number of entries in the menu table
    //   blankLineBeforePreMenu: Display a blank line before the preMenu
    //   blankLineBeforeMenuHeader: Display a blank line before the menu header
    //   blankLineAfterMenuHeader: Display a blank line after the menu header
    //   alignCommands: Align the commands
    //   blankLineAfterMenu: Display a blank line after the menu
    R4A_TELNET_CLIENT(const R4A_MENU_TABLE * menuTable,
                      int menuTableEntries,
                      bool blankLineBeforePreMenu = true,
                      bool blankLineBeforeMenuHeader = true,
                      bool blankLineAfterMenuHeader = false,
                      bool alignCommands = true,
                      bool blankLineAfterMenu = false)
        : _menu{R4A_MENU(menuTable, menuTableEntries, blankLineBeforePreMenu,
                blankLineBeforeMenuHeader, blankLineAfterMenuHeader,
                alignCommands, blankLineAfterMenu)}
    {
    }

    // Allow the R4A_TELNET_SERVER access to the private members
    friend class R4A_TELNET_SERVER;
};

//****************************************
// Telnet Menu API
//****************************************

extern const R4A_MENU_ENTRY r4aTelnetMenuTable[]; // Telnet menu table
#define R4A_TELNET_MENU_ENTRIES       4           // Telnet menu table entries

// Display the telnet clients
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aTelnetMenuClients(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

// Display the telnet options
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aTelnetMenuOptions(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

// Display the telnet options
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aTelnetMenuState(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display);

// Display the telnet state
// Inputs:
//   display: Device used for output
void r4aTelnetMenuStateDisplay(Print * display);

//****************************************
// Telnet Server API
//****************************************

// Telnet server
class R4A_TELNET_SERVER : WiFiServer
{
private:
    enum R4A_TELNET_SERVER_STATE
    {
        R4A_TELNET_STATE_OFF = 0,
        R4A_TELNET_STATE_ALLOCATE_CLIENT,
        R4A_TELNET_STATE_LISTENING,
        R4A_TELNET_STATE_SHUTDOWN,
    };
    uint8_t _state; // Telnet server state
    uint16_t _port; // Port number for the telnet server
    bool _echo;
    R4A_TELNET_CLIENT * _newClient; // New client object, ready for listen call
    R4A_TELNET_CLIENT * _clientList; // Singlely linked list of telnet clients
    const R4A_MENU_TABLE * const _menuTable; // Address of all menu descriptions
    const int _menuTableEntries;             // Number of entries in the menu table
    const bool _blankLineBeforePreMenu;      // Display a blank line before the preMenu
    const bool _blankLineBeforeMenuHeader;   // Display a blank line before the menu header
    const bool _blankLineAfterMenuHeader;    // Display a blank line after the menu header
    const bool _alignCommands;               // Align the commands
    const bool _blankLineAfterMenu;          // Display a blank line after the menu

public:

    Print * _debugState; // Address of Print object to display telnet server state changes
    Print * _displayOptions; // Address of Print object to display telnet options

    // Constructor
    // Inputs:
    //   menuTable: Address of table containing the menu descriptions, the
    //              main menu must be the first entry in the table.
    //   menuEntries: Number of entries in the menu table
    //   blankLineBeforePreMenu: Display a blank line before the preMenu
    //   blankLineBeforeMenuHeader: Display a blank line before the menu header
    //   blankLineAfterMenuHeader: Display a blank line after the menu header
    //   alignCommands: Align the commands
    //   blankLineAfterMenu: Display a blank line after the menu
    //   port: Port number for internet connection
    //   echo: When true causes the input characters to be echoed
    R4A_TELNET_SERVER(const R4A_MENU_TABLE * menuTable,
                      int menuTableEntries,
                      bool blankLineBeforePreMenu = true,
                      bool blankLineBeforeMenuHeader = true,
                      bool blankLineAfterMenuHeader = false,
                      bool alignCommands = true,
                      bool blankLineAfterMenu = false,
                      uint16_t port = 23,
                      bool echo = false)
        : _menuTable{menuTable}, _menuTableEntries{menuTableEntries},
          _state{0}, _port{port}, _echo{echo}, _newClient{nullptr},
          _clientList{nullptr}, _debugState{nullptr}, _displayOptions{nullptr},
          _blankLineBeforePreMenu{blankLineBeforePreMenu},
          _blankLineBeforeMenuHeader{blankLineBeforeMenuHeader},
          _blankLineAfterMenuHeader{blankLineAfterMenuHeader},
          _alignCommands{alignCommands}, _blankLineAfterMenu{blankLineAfterMenu},
          WiFiServer()
    {
    }

    // Destructor
    ~R4A_TELNET_SERVER()
    {
        if (_newClient)
            delete _newClient;
        while (_clientList)
        {
            R4A_TELNET_CLIENT * client = _clientList;
            _clientList = client->_nextClient;
            client->stop();
            delete client;
        }
    }

    // Display the client list
    void displayClientList(Print * display = &Serial);

    // Get the telnet port number
    //  Returns the telnet port number set during initalization
    uint16_t port();

    // Start and update the telnet server
    // Inputs:
    //   wifiConnected: Set true when WiFi is connected to an access point
    //                  and false otherwise
    void update(bool wifiConnected);
};

extern class R4A_TELNET_SERVER telnet;  // Server providing telnet access

//****************************************
// Time Zone API
//****************************************

extern char r4aTimeZoneHours;
extern char r4aTimeZoneMinutes;
extern char r4aTimeZoneSeconds;

#endif  // __R4A_ROBOT_H__
