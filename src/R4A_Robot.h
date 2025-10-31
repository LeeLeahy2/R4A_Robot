/**********************************************************************
  R4A_Robot.h

  Robots-For-All (R4A)
  Declare the robot support
**********************************************************************/

#ifndef __R4A_ROBOT_H__
#define __R4A_ROBOT_H__

#include <Arduino.h>            // Built-in
#include <base64.h>             // Built-in, needed for NTRIP Client credential encoding
#include <BluetoothSerial.h>    // Built-in
#include <byteswap.h>           // Built-in
#include <esp32-hal-spi.h>      // Built-in
#include <math.h>               // Built-in
#include <Network.h>            // Built-in
#include <WiFi.h>               // Built-in
#include <WiFiMulti.h>          // Built-in
#include <WiFiServer.h>         // Built-in

// External libraries
#include <NTPClient.h>          // https://github.com/arduino-libraries/NTPClient
#include <TimeLib.h>            // In Time library, format and parse time values

#pragma GCC diagnostic ignored "-Wreorder"

//****************************************
// Constants
//****************************************

// Define distance constants
#define R4A_MILLIMETERS_PER_METER       ((double)1000.)
#define R4A_CENTIMETERS_PER_METER       ((double)100.)
#define R4A_METERS_PER_KILOMETER        ((double)1000.)
#define R4A_MILLIMETERS_PER_KILOMETER   (MILLIMETERS_PER_METER * METERS_PER_KILOMETER)

#define R4A_INCHES_PER_FOOT             ((double)12.)
#define R4A_FEET_PER_MILE               ((double)5280.)

#define R4A_MILLIMETERS_PER_INCH        ((double)25.4)
#define R4A_CENTIMETERS_PER_INCH        ((double)2.54)
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

// Earth radius: https://www.google.com/search?q=earth+radius
#define R4A_EARTH_AVE_RADIUS_KM         6371
#define R4A_EARTH_EQUATORIAL_RADIUS_KM  6378
#define R4A_EARTH_POLE_RADIUS_KM        6357

// Frequency constants
#define R4A_FREQ_KHz        1000
#define R4A_FREQ_MHz        (1000 * R4A_FREQ_KHz)
#define R4A_FREQ_GHz        (1000 * R4A_FREQ_MHz)

//****************************************
// Atomic support
//****************************************

// Perform an operation on an object
// Inputs:
//   obj: Address of an object to be adjusted
//   value: Value used in the operation
//   moBefore: Memory order to use before the operation
// Returns:
//   Returns the value before the operation
int32_t r4aAtomicAdd32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicAnd32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicExchange32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicOr32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicSub32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);
int32_t r4aAtomicXor32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);

// Perform an operation on an object
// Inputs:
//   obj: Address of an object to be compared and modified
//   expected: Address of an object for the comparison
//   value: Value to store in obj if the comparison is successful
//   unknown:
//   moSuccess: Memory order to use when the comparison is successful
//   moFailure: Memory order to use after comparison failure
// Returns:
//   Returns the true if the two objects were equal and false otherwise
bool r4aAtomicCompare32(int32_t * obj,
                        int32_t * expected,
                        int32_t value,
                        bool unknown = false,
                        int moBefore = __ATOMIC_RELAXED,
                        int moAfter = __ATOMIC_RELAXED);

// Get the value of an atomic object
// Inputs:
//   obj: Address of an object to be read
//   moBefore: Memory order to use before the operation
// Returns:
//   Returns the value contained in the object
int32_t r4aAtomicLoad(int32_t * obj, int moBefore = __ATOMIC_RELAXED);

// Set the value of an atomic object
// Inputs:
//   obj: Address of an object to be written
//   value: Object's new value
//   moBefore: Memory order to use before the operation
// Returns:
//   Returns the value before the operation
void r4aAtomicStore32(int32_t * obj, int32_t value, int moBefore = __ATOMIC_RELAXED);

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
// GNSS API
//****************************************

// Earth radius in meters
#define R4A_GNSS_EARTH_AVE_RADIUS_MPD   (R4A_EARTH_AVE_RADIUS_KM        \
                                         * R4A_METERS_PER_KILOMETER     \
                                         / 360)

#define R4A_GNSS_EARTH_LAT_RADIUS_MPD   (R4A_EARTH_POLE_RADIUS_KM       \
                                         * R4A_METERS_PER_KILOMETER     \
                                         / 360)

#define R4A_GNSS_EARTH_LONG_RADIUS_MPD  (R4A_EARTH_EQUATORIAL_RADIUS_KM \
                                       * R4A_METERS_PER_KILOMETER       \
                                       / 360)

// Earth radius in centimeters
#define R4A_GNSS_EARTH_AVE_RADIUS_CPD   (R4A_GNSS_EARTH_AVE_RADIUS_MPD  \
                                         * R4A_CENTIMETERS_PER_METER)

#define R4A_GNSS_EARTH_LAT_RADIUS_CPD   (R4A_GNSS_EARTH_LAT_RADIUS_MPD  \
                                         * R4A_CENTIMETERS_PER_METER)

#define R4A_GNSS_EARTH_LONG_RADIUS_CPD  (R4A_GNSS_EARTH_LONG_RADIUS_MPD \
                                         * R4A_CENTIMETERS_PER_METER)

// Earth radius in inches
#define R4A_GNSS_EARTH_AVE_RADIUS_IPD   (R4A_GNSS_EARTH_AVE_RADIUS_CPD  \
                                         / R4A_CENTIMETERS_PER_INCH)

#define R4A_GNSS_EARTH_LAT_RADIUS_IPD   (R4A_GNSS_EARTH_LAT_RADIUS_CPD  \
                                         / R4A_CENTIMETERS_PER_INCH)

#define R4A_GNSS_EARTH_LONG_RADIUS_IPD  (R4A_GNSS_EARTH_LONG_RADIUS_CPD \
                                       / R4A_CENTIMETERS_PER_INCH)

// Degrees per meter
#define R4A_GNSS_AVE_DPM    (360. / (R4A_EARTH_AVE_RADIUS_KM    \
                                     * R4A_METERS_PER_KILOMETER))

#define R4A_GNSS_LAT_DPM    (360. / (R4A_EARTH_POLE_RADIUS_KM   \
                                     * R4A_METERS_PER_KILOMETER))

#define R4A_GNSS_LONG_DPM   (360. / (R4A_EARTH_EQUATORIAL_RADIUS_KM \
                                     * R4A_METERS_PER_KILOMETER))

// Degrees per centimeter
#define R4A_GNSS_AVE_DPC    (R4A_GNSS_AVE_DPM / R4A_CENTIMETERS_PER_METER)

#define R4A_GNSS_LAT_DPC    (R4A_GNSS_LAT_DPM / R4A_CENTIMETERS_PER_METER)

#define R4A_GNSS_LONG_DPC   (R4A_GNSS_LONG_DPM / R4A_CENTIMETERS_PER_METER)

// Degrees per inch

#define R4A_GNSS_AVE_DPI    (R4A_GNSS_AVE_DPC * R4A_CENTIMETERS_PER_INCH)

#define R4A_GNSS_LAT_DPI    (R4A_GNSS_LAT_DPC * R4A_CENTIMETERS_PER_INCH)

#define R4A_GNSS_LONG_DPI   (R4A_GNSS_LONG_DPC * R4A_CENTIMETERS_PER_INCH)

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

#define R4A_LED_BLUE_SHIFT              0
#define R4A_LED_GREEN_SHIFT             8
#define R4A_LED_RED_SHIFT               16
#define R4A_LED_WHITE_SHIFT             24

extern const struct _R4A_SPI_DEVICE * r4aLEDSpi;

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
//   spiDevice: Address of an R4A_SPI_DEVICE data structure
//   numberOfLEDs: Number of multi-color LEDs in the string
// Outputs:
//   Returns true for successful initialization and false upon error
bool r4aLEDSetup(const struct _R4A_SPI_DEVICE * spiDevice,
                 uint8_t numberOfLEDs);

// Turn off the LEDs
void r4aLEDsOff();

// Update the colors on the WS2812 LEDs
// Inputs:
//   updateRequest: When true causes color data to be sent to the LEDS
//                  When false only updates LEDs if color or intensity was changed
//   display: Address of the Print object for output
void r4aLEDUpdate(bool updateRequest, Print * display = nullptr);

//****************************************
// Lock API
//****************************************

// Take out a lock
// Inputs:
//   lock: Address of the lock
void r4aLockAcquire(volatile int32_t * lock, int moBefore = __ATOMIC_RELAXED, int moAfter = __ATOMIC_RELAXED);

// Release a lock
// Inputs:
//   lock: Address of the lock
void r4aLockRelease(volatile int32_t * lock, int moBefore = __ATOMIC_RELAXED);

//****************************************
// Memory API
//****************************************

extern bool r4aMallocDebug;

// User defined delete function, see https://en.cppreference.com/w/cpp/memory/new/operator_delete
// Inputs:
//   ptr: Address of the buffer to free
//   text: Address of zero terminated string of characters
void operator delete(void * ptr, const char * text) noexcept;

// User defined new function, see https://en.cppreference.com/w/cpp/memory/new/operator_new
// Inputs:
//   numberOfBytes: Number of bytes to allocate for the buffer
//   text: Address of zero terminated string of characters
// Outputs:
//   Returns the buffer address when successful or nullptr if failure
void* operator new(std::size_t numberOfBytes, const char * text);

// Free a DMA buffer, set the pointer to nullptr after it is freed
// Inputs:
//   buffer: Address of the buffer to be freed
//   text: Text to display when debugging is enabled
void r4aDmaFree(void * buffer, const char * text);

// Allocate a buffer that support DMA
// Inputs:
//   numberOfBytes: Number of bytes to allocate
//   text: Text to display when debugging is enabled
// Outputs:
//   Returns the buffer address or nullptr upon failure
void * r4aDmaMalloc(size_t bytes, const char * text);

// Free a buffer, set the pointer to nullptr after it is freed
// Inputs:
//   buffer: Address of the buffer to be freed
//   text: Text to display when debugging is enabled
void r4aFree(void * buffer, const char * text);

// Allocate a buffer
// Inputs:
//   numberOfBytes: Number of bytes to allocate
//   text: Text to display when debugging is enabled
// Outputs:
//   Returns the buffer address or nullptr upon failure
void * r4aMalloc(size_t numberOfBytes, const char * text);

// Determine the memory location
// Inputs:
//   addr: Address of the buffer
// Outputs:
//   Returns a zero terminated string containing the location of the buffer
const char * r4aMemoryLocation(void * addr);

//****************************************
// Menu API
//****************************************

#define R4A_MENU_NONE           0
#define R4A_MENU_MAIN           1

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
    const char * command;         // Command: Expected serial input
    R4A_MENU_ROUTINE menuRoutine; // Routine to process the menu
    intptr_t menuParameter;       // Parameter for the menu routine
    R4A_HELP_ROUTINE helpRoutine; // Routine to display the help message
    int align;                    // Command length adjustment for alignment
    const char * helpText;        // Help text to display
} R4A_MENU_ENTRY;

// Forward declarations
// Inputs:
//   display: Address of the Print object for output
// Outputs:
//   Returns true if successful and false to force return to main menu
typedef bool (*R4A_PRE_MENU_DISPLAY)(Print * display);

typedef struct _R4A_MENU_TABLE
{
    // Menu description
    const char * menuName;             // Name of the menu
    R4A_PRE_MENU_DISPLAY preMenu;      // Routine to display data before the menu
    const R4A_MENU_ENTRY * firstEntry; // First entry in the menu
    int32_t menuEntryCount;            // Number of entries in the table
} R4A_MENU_TABLE;

typedef struct _R4A_MENU
{
    // private
    const R4A_MENU_TABLE * _menu;      // Current menu to display and use
    const R4A_MENU_TABLE * _menuTable; // Address of all menu descriptions
    int _menuTableEntries;             // Number of entries in the menu table

    // Menu formatting
    bool _blankLineBeforePreMenu;    // Display a blank line before the preMenu
    bool _blankLineBeforeMenuHeader; // Display a blank line before the menu header
    bool _blankLineAfterMenuHeader;  // Display a blank line after the menu header
    bool _alignCommands;             // Align the commands
    bool _blankLineAfterMenu;        // Display a blank line after the menu

    // public
    bool _debug; // Set true to enable debugging
} R4A_MENU;

// Initialize the R4A_MENU data structure
// Inputs:
//   menu: Address of an R4A_MENU data structure
//   menuTable: Address of table containing the menu descriptions, the
//              main menu must be the first entry in the table.
//   menuEntries: Number of entries in the menu table
//   blankLineBeforePreMenu: Display a blank line before the preMenu
//   blankLineBeforeMenuHeader: Display a blank line before the menu header
//   blankLineAfterMenuHeader: Display a blank line after the menu header
//   alignCommands: Align the commands
//   blankLineAfterMenu: Display a blank line after the menu
void r4aMenuBegin(R4A_MENU * menu,
                  const R4A_MENU_TABLE * menuTable,
                  int menuEntries,
                  bool blankLineBeforePreMenu = true,
                  bool blankLineBeforeMenuHeader = true,
                  bool blankLineAfterMenuHeader = false,
                  bool alignCommands = true,
                  bool blankLineAfterMenu = false);

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

// Display the menu object contents
// Inputs:
//   menu: Address of an R4A_MENU data structure
//   display: Address of the Print object for output
void r4aMenuDisplay(R4A_MENU * menu, Print * display);

// Get the string of parameters
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
String r4aMenuGetParameters(const struct _R4A_MENU_ENTRY * menuEntry,
                            const char * command);

// Display the menu item with a suffix and help text
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   align: Zero terminated string of spaces for alignment
//   display: Device used for output
void r4aMenuHelpSuffix(const struct _R4A_MENU_ENTRY * menuEntry,
                       const char * align,
                       Print * display);

// Determine if the menu system is active
// Outputs:
//   Returns true when the menu system is active and false when inactive
bool r4aMenuIsActive();

// Process a menu command when specified or display the menu when command
// is nullptr.
// Inputs:
//   menu: Address of an R4A_MENU data structure
//   menuTable: Address of the address of the menuTable (description of the menu)
//   command: Command string
//   display: Address of the Print object for output
// Outputs:
//   True when exiting the menu system, false if still in the menu system
bool r4aMenuProcess(R4A_MENU * menu,
                    const char * command,
                    Print * display = &Serial);

//****************************************
// LED Menu API
//****************************************

extern const R4A_MENU_ENTRY r4aLEDMenuTable[];
#define R4A_LED_MENU_ENTRIES    6

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
[[deprecated("Use r4aMenuHelpSuffix instead.")]]
void r4aLEDMenuHelpiii(const struct _R4A_MENU_ENTRY * menuEntry,
                       const char * align,
                       Print * display);

// Display the help text with ll and cccccccc
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   align: Zero terminated string of spaces for alignment
//   display: Device used for output
[[deprecated("Use r4aMenuHelpSuffix instead.")]]
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
extern bool r4aNtpOnline; // Set true while client is connected to NTP server

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

// Minimum number of bytes to process
#define R4A_NTRIP_CLIENT_MINIMUM_RX_BYTES       32

// Define the back-off intervals between connection attempts in milliseconds
extern const uint32_t r4aNtripClientBbackoffIntervalMsec[];
extern const int r4aNtripClientBbackoffCount;

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

typedef struct _R4A_NTRIP_CLIENT
{
    // The network connection to the NTRIP caster to obtain RTCM data.
    NetworkClient * _client;
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
} R4A_NTRIP_CLIENT;

extern R4A_NTRIP_CLIENT r4aNtripClient;

// Display the response from the NTRIP caster
// Inputs:
//   response: Address of a buffer containing the NTRIP caster response
//   bytesRead: Number of bytes received from the NTRIP caster
//   display: Print object address used to display output
void r4aNtripClientDisplayResponse(char * response,
                                   int bytesRead,
                                   Print * display);

// Get the I2C bus transaction size
// Outputs:
//   Returns the maximum I2C transaction size in bytes
uint8_t r4aNtripClientI2cTransactionSize();

// Print the NTRIP client state summary
// Inputs:
//   display: Print object address used to display output
void r4aNtripClientPrintStateSummary(Print * display = &Serial);

// Print the NTRIP Client status
// Inputs:
//   display: Print object address used to display output
void r4aNtripClientPrintStatus(Print * display = &Serial);

// Send data to the GNSS radio
// Inputs:
//   buffer: Address of a buffer containing the data to push to the GNSS
//   bytesToPush: Number of bytes to push to the GNSS
// Outputs:
//   Returns the number of bytes successfully pushed to the GNSS
int r4aNtripClientPushRawData(uint8_t * buffer,
                              int bytesToPush);

// Remove data from the ring buffer
// Inputs:
//   display: Print object address used to display output
// Outputs:
//   Returns the number of bytes removed from the ring buffer
int r4aNtripClientRbRemoveData(Print * display = &Serial);

// Check for the arrival of any correction data. Push it to the GNSS.
// Stop task if the connection has dropped or if we receive no data for
// _receeiveTimeout
// Inputs:
//   wifiConnected: True when WiFi is connected to a remote AP, false
//                  otherwise
//   display: Print object address used to display output
void r4aNtripClientUpdate(bool wifiConnected, Print * display = nullptr);

// Verify the NTRIP client tables
void r4aNtripClientValidateTables();

//****************************************
// NTRIP Client Menu API
//****************************************

extern const R4A_MENU_ENTRY r4aNtripClientMenuTable[];
#define R4A_NTRIP_CLIENT_MENU_ENTRIES   7   // NTRIP client menu table entries

//****************************************
// ReadLine API
//****************************************

// Read a line of input from a Bluetooth serial port into a character array
// Inputs:
//   echo: Specify true to enable echo of input characters and false otherwise
//   buffer: Address of a string that contains the input line
//   port: Address of a Bluetooth serial port structure
// Outputs:
//   nullptr when the line is not complete
String * r4aReadLine(bool echo, String * buffer, BluetoothSerial * port);

// Read a line of input from a serial port into a character array
// Inputs:
//   echo: Specify true to enable echo of input characters and false otherwise
//   buffer: Address of a string that contains the input line
//   port: Address of a HardwareSerial port structure
// Outputs:
//   nullptr when the line is not complete
String * r4aReadLine(bool echo, String * buffer, HardwareSerial * port = &Serial);

// Read a line of input from a WiFi client into a character array
// Inputs:
//   echo: Specify true to enable echo of input characters and false otherwise
//   buffer: Address of a string that contains the input line
//   port: Address of a NetworkClient port structure
// Outputs:
//   nullptr when the line is not complete
String * r4aReadLine(bool echo, String * buffer, NetworkClient * port);

//****************************************
// Robot Challenge API
//****************************************

#define R4A_CHALLENGE_SEC_LIGHT_TRACKING    (3 * R4A_SECONDS_IN_A_MINUTE)
#define R4A_CHALLENGE_SEC_LINE_FOLLOWING    (3 * R4A_SECONDS_IN_A_MINUTE)
#define R4A_CHALLENGE_SEC_WAYPOINT_FOLLOWING (15 * R4A_SECONDS_IN_A_MINUTE)

#define R4A_CHALLENGE_SEC_START_DELAY       5   // Seconds

// The robotRunning routine calls this routine to actually perform
// the challenge.  This routine typically reads a sensor and may
// optionally adjust the motors based upon the sensor reading.  The
// routine then must return.  The robot layer will call this routine
// multiple times during the robot operation.
// Inputs:
//   object: Address of a R4A_ROBOT_CHALLENGE data structure
typedef void (* R4A_ROBOT_CHALLENGE_ROUTINE)(struct _R4A_ROBOT_CHALLENGE * object);

// Initialize the robot challenge
// The robotStart calls this routine before switching to the initial
// delay state.
// Inputs:
//   object: Address of a R4A_ROBOT_CHALLENGE data structure
typedef void (* R4A_ROBOT_CHALLENGE_INIT)(struct _R4A_ROBOT_CHALLENGE * object);

// Start the robot challenge
// The initial delay routine calls this routine just before calling
// the challenge routine for the first time.
// Inputs:
//   object: Address of a R4A_ROBOT_CHALLENGE data structure
typedef void (* R4A_ROBOT_CHALLENGE_START)(struct _R4A_ROBOT_CHALLENGE * object);

// The robotStop routine calls this routine to stop the motors and
// perform any other actions.
// Inputs:
//   object: Address of a R4A_ROBOT_CHALLENGE data structure
typedef void (* R4A_ROBOT_CHALLENGE_STOP)(struct _R4A_ROBOT_CHALLENGE * object);

typedef struct _R4A_ROBOT_CHALLENGE
{
    // Constants, DO NOT MODIFY, set during structure initialization
    R4A_ROBOT_CHALLENGE_ROUTINE _challenge;
    R4A_ROBOT_CHALLENGE_INIT _init;
    R4A_ROBOT_CHALLENGE_START _start;
    R4A_ROBOT_CHALLENGE_STOP _stop;

    const char * _name; // Name of the challenge
    uint32_t _duration; // Number of seconds to run the robot challenge
} R4A_ROBOT_CHALLENGE;

//****************************************
// Robot API
//****************************************

enum ROBOT_STATES
{
    ROBOT_STATE_IDLE = 0,   // The robot layer is idle
    ROBOT_STATE_COUNT_DOWN, // The robot layer is counting down to start
    ROBOT_STATE_RUNNING,    // The robot layer is running the challenge
};

// Display the delta time
// Inputs:
//   deltaMsec: Milliseconds to display
typedef void (* R4A_ROBOT_TIME_CALLBACK)(uint32_t deltaMsec);

typedef struct _R4A_ROBOT
{
    // Private

    volatile bool _busy;        // True while challenge is running
    volatile R4A_ROBOT_CHALLENGE * _challenge;  // Address of challenge object
    int _core;                  // CPU core number
    uint32_t _endMsec;          // Challenge end time in milliseconds since boot
    uint32_t _initMsec;         // Challenge init time in milliseconds since boot
    uint32_t _nextDisplayMsec;  // Next time display time should be called in milliseconds since boot
    uint32_t _startDelayMsec;   // Number of milliseconds before starting the challenge
    uint32_t _startMsec;        // Challenge start time in milliseconds since boot
    uint32_t _stopMsec;         // Challenge stop time in milliseconds since boot
    volatile uint8_t _state;    // Next state for robot operation

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
} R4A_ROBOT;

// Get the runtime in milliseconds
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   currentMsec: Current time in milliseconds since boot
// Outputs:
//   Returns the delta time in milliseconds
uint32_t r4aRobotGetRunTime(R4A_ROBOT * robot, uint32_t currentMsec);

// Get the challenge stop time in milliseconds since boot
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
// Outputs:
//   Returns the challenge stop time in milliseconds
uint32_t r4aRobotGetStopTime(R4A_ROBOT * robot);

// Initialize the robot data structure
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   core: CPU core that is running the robot layer
//   idle: Address of the idle routine, may be nullptr
//   displayTime: Address of the routine to display the time, may be nullptr
void r4aRobotInit(R4A_ROBOT * robot,
                  int core = 0,
                  R4A_ROBOT_TIME_CALLBACK idle = nullptr,
                  R4A_ROBOT_TIME_CALLBACK displayTime = nullptr);

// Determine if the robot layer is active
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
// Outputs:
//   Returns true when the challenge is running and false otherwise
bool r4aRobotIsActive(R4A_ROBOT * robot);

// Determine if the robot layer is running
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
// Outputs:
//   Returns true when the robot is in the running state and false otherwise
bool r4aRobotIsRunning(R4A_ROBOT * robot);

// Determine if it is possible to start the robot
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   challenge: Address of challenge object
//   startDelaySec: Number of seconds before the robot starts a challenge
//   display: Device used for output
bool r4aRobotStart(R4A_ROBOT * robot,
                   R4A_ROBOT_CHALLENGE * challenge,
                   uint32_t startDelaySec = 5,
                   Print * display = &Serial);

// Stop the robot
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   currentMsec: Milliseconds since boot
//   display: Device used for output
void r4aRobotStop(R4A_ROBOT * robot,
                  uint32_t currentMsec,
                  Print * display = &Serial);

// Update the robot state
// Inputs:
//   robot: Address of an R4A_ROBOT data structure
//   currentMsec: Milliseconds since boot
void r4aRobotUpdate(R4A_ROBOT * robot,
                    uint32_t currentMsec);

//****************************************
// Runtime library support
//****************************************

// Swap the two bytes
uint16_t r4aBswap16(uint16_t value);

// Swap the four bytes
uint32_t r4aBswap32(uint32_t value);

// Compare two strings ignoring case
// Inputs:
//   str1: Address of a zero terminated string of characters
//   str2: Address of a zero terminated string of characters
// Outputs:
//   Returns the delta value of the last comparison (str1[x] - str2[x])
int r4aStricmp(const char *str1, const char *str2);

// Compare two strings limited to N characters ignoring case
// Inputs:
//   str1: Address of a zero terminated string of characters
//   str2: Address of a zero terminated string of characters
//   length: Maximum number of characters to compare
// Outputs:
//   Returns the delta value of the last comparison (str1[x] - str2[x])
int r4aStrincmp(const char *str1, const char *str2, int length);

//****************************************
// Serial API
//****************************************

// Display an error message on a regular interval
void r4aReportErrorMessage(const char * errorMessage,
                           Print * display = &Serial);

// Repeatedly display a fatal error message
// Inputs:
//   errorMessage: Zero terminated string of characters containing the
//                 error mesage to be displayed
//   display: Device used for output
void r4aReportFatalError(const char * errorMessage,
                         Print * display = &Serial);

// Process Bluetooth menu item
// Inputs:
//   menu: Address of the menu object
//   port: Address of a BluetoothSerial object
// Outputs:
//   Returns true when the client exits the menu system and false otherwise
bool r4aBluetoothMenu(R4A_MENU * menu, BluetoothSerial * port);

// Process serial menu item
// Inputs:
//   menu: Address of the menu object
void r4aSerialMenu(R4A_MENU * menu);

//****************************************
// SPI API
//****************************************

// Transfer data to the SPI device
// Inputs:
//   spiBus: Address of an R4A_SPI_BUS data structure
//   txDmaBuffer: Address of the DMA buffer containing the data to send
//   rxDmaBuffer: Address of the receive DMA data buffer
//   length: Number of data bytes to transfer
//   display: Address of Print object for output, maybe nullptr
// Outputs:
//   Return true if successful and false upon failure
typedef bool (* R4A_SPI_TRANSFER)(struct _R4A_SPI_BUS * spiBus,
                                  const uint8_t * txDmaBuffer,
                                  uint8_t * rxDmaBuffer,
                                  size_t length,
                                  Print * display);

// Describe the SPI bus
typedef struct _R4A_SPI_BUS
{
    uint8_t _busNumber;         // Number of the SPI bus
    int8_t _pinSCLK;            // GPIO number connected to SPI clock pin, -1 = not connected
    int8_t _pinMOSI;            // GPIO number connected to SPI MOSI pin, -1 = not connected
    int8_t _pinMISO;            // GPIO number connected to SPI MISO pin, -1 = not connected
    R4A_SPI_TRANSFER _transfer; // Routine to perform a SPI transfer
} R4A_SPI_BUS;

typedef struct _R4A_SPI_DEVICE
{
    R4A_SPI_BUS * _spiBus;  // Address of R4A_SPI data structure
    uint32_t _clockHz;      // SPI clock frequency for device
    int8_t _pinCS;          // GPIO number connected to chip select pin of device, -1 = not connected
    bool _chipSelectValue;  // 0 or 1, value to enable chip operations
    bool _clockPolarity;    // 0 or 1
    bool _clockPhase;       // 0 or 1
} R4A_SPI_DEVICE;

// Transfer the data to the SPI device
// Inputs:
//   spiDevice: Address of an R4A_SPI_DEVICE data structure
//   txBuffer: Address of the buffer containing the data to send
//   rxBuffer: Address of the receive data buffer
//   length: Number of data bytes to transfer
//   display: Address of Print object for output, maybe nullptr
// Outputs:
//   Return true if successful and false upon failure
bool r4aSpiTransfer(const R4A_SPI_DEVICE * spiDevice,
                    const uint8_t * txBuffer,
                    uint8_t * rxBuffer,
                    size_t length,
                    Print * display = nullptr);

//****************************************
// SPI Flash API
//****************************************

#define R4A_SPI_FLASH_9F_ID_BYTES       3

// Routine to update the write protect pin state, use when a GPIO is not connected
// Inputs:
//   enable: Set true to enable writes to the chip and false to protect the chip
// Outputs:
//   Returns true if the write protection was set properly
typedef bool (* R4A_SPI_FLASH_WRITE_ENABLE_PIN_STATE)(bool enable);

// Routine to decode and display the SPI Flash status register value
// Inputs:
//   status: SPI Flash status register value
//   display: Address of Print object for output
typedef void (* R4A_SPI_FLASH_DISPLAY_STATUS)(uint8_t status, Print * display);

typedef struct _R4A_SPI_FLASH_PROTECTION
{
    uint32_t _flashAddress;
    int8_t _readProtectBit;
    int8_t _writeProtectBit;
} R4A_SPI_FLASH_PROTECTION;

typedef struct _R4A_SPI_FLASH
{
    R4A_SPI_DEVICE _flashChip;  // SPI NOR flash device configuration
    int8_t _pinHold;            // GPIO pin number for the HOLD# pin
    int8_t _pinWriteProtect;    // GPIO pin number for the WP# pin
    R4A_SPI_FLASH_WRITE_ENABLE_PIN_STATE _writeEnablePinState;
    R4A_SPI_FLASH_DISPLAY_STATUS _displayStatus;
    const R4A_SPI_FLASH_PROTECTION * _blockProtect; // Block protection table
    uint32_t _flashBytes;           // Flash size in bytes
    uint8_t _blockProtectBytes;     // Length in bytes of the block protection register
    uint8_t _stsWriteInProgress;    // Status bit indicating write activity
    uint8_t _stsEraseErrors;        // Status bits indicating erase errors
    uint8_t _stsProgramErrors;      // Status bits indicating program errors
} R4A_SPI_FLASH;

extern const R4A_SPI_FLASH * r4aSpiFlash;

// Initialize the connection to the SPI flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
bool r4aSpiFlashBegin(const R4A_SPI_FLASH * spiFlash);

// Enable/disable read access to the block containing the specified address
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   flashAddress: Address within the block to protect
//   enable: Set true to enable writes and false to prevent writes
//   status: Address to receive the final write status
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write access was successfully updated and false upon error
bool r4aSpiFlashBlockReadProtection(const R4A_SPI_FLASH * spiFlash,
                                    uint32_t flashAddress,
                                    bool enable,
                                    uint8_t * status,
                                    Print * display);

// Enable/disable block read access
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   enable: Set true to enable writes and false to prevent writes
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write access was successfully updated and false upon error
bool r4aSpiFlashBlockReadProtectionAll(const R4A_SPI_FLASH * spiFlash,
                                       bool enable,
                                       Print * display);

// Enable/disable write access to the block containing the specified address
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   flashAddress: Address within the block to protect
//   enable: Set true to enable writes and false to prevent writes
//   status: Address to receive the final write status
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write access was successfully updated and false upon error
bool r4aSpiFlashBlockWriteProtection(const R4A_SPI_FLASH * spiFlash,
                                     uint32_t flashAddress,
                                     bool enable,
                                     uint8_t * status,
                                     Print * display);

// Enable/disable block write access
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   enable: Set true to enable writes and false to prevent writes
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write access was successfully updated and false upon error
bool r4aSpiFlashBlockWriteProtectionAll(const R4A_SPI_FLASH * spiFlash,
                                     bool enable,
                                     Print * display);

// Allocate the DMA buffer
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
// Outputs:
//   Returns true if the DMA buffer was allocated and false upon failure
bool r4aSpiFlashDmaBufferAllocate(const R4A_SPI_FLASH * spiFlash);

// Release the DMA buffer
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
void r4aSpiFlashDmaBufferRelease(const R4A_SPI_FLASH * spiFlash);

// Erase a block (8K, 32K or 65K bytes) from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   flashAddress: Address within the block to erase
//   status: Address of buffer to receive the final status
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the block erase was successful and false upon error
bool r4aSpiFlashEraseBlock(const R4A_SPI_FLASH * spiFlash,
                           uint32_t flashAddress,
                           uint8_t * status,
                           Print * display);

// Erase the entire SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   status: Address to receive the final write status
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the erase was successful and false upon error
bool r4aSpiFlashEraseChip(const R4A_SPI_FLASH * spiFlash,
                          uint8_t * status,
                          Print * display);

// Erase a sector (4K bytes) from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   flashAddress: Address within the sector to erase
//   status: Address of buffer to receive the final status
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the subsector erase was successful and false upon error
bool r4aSpiFlashEraseSector(const R4A_SPI_FLASH * spiFlash,
                            uint32_t flashAddress,
                            uint8_t * status,
                            Print * display);

// Enable/disable read access to block containing the specified address
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionRead(const R4A_MENU_ENTRY * menuEntry,
                                        const char * command,
                                        Print * display);

// Enable/disable read access to all blocks
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionReadAll(const R4A_MENU_ENTRY * menuEntry,
                                           const char * command,
                                           Print * display);

// Read block protection register from the SPI flash device
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionStatus(const R4A_MENU_ENTRY * menuEntry,
                                          const char * command,
                                          Print * display);

// Enable/disable write access to block containing the specified address
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionWrite(const R4A_MENU_ENTRY * menuEntry,
                                         const char * command,
                                         Print * display);

// Enable/disable write access to all blocks
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionWriteAll(const R4A_MENU_ENTRY * menuEntry,
                                            const char * command,
                                            Print * display);

// Erase a 4K block of the SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuErase4K(const R4A_MENU_ENTRY * menuEntry,
                            const char * command,
                            Print * display);

// Erase a 65K block of the SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuErase65K(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display);

// Erase the entire SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuEraseChip(const R4A_MENU_ENTRY * menuEntry,
                              const char * command,
                              Print * display);

// Read data from the SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadData(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display);

// Read ID from the SPI flash device
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadId9e(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display);

// Read ID from the SPI flash device
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadId9f(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display);

// Read the discovery parameters from the SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadParameters(const R4A_MENU_ENTRY * menuEntry,
                                   const char * command,
                                   Print * display);

// Read and display the status register
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadStatusRegister(const R4A_MENU_ENTRY * menuEntry,
                                       const char * command,
                                       Print * display);

// Enable or disable SPI NOR Flash writes using the WP# pin
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aSpiFlashMenuWriteEnable(const R4A_MENU_ENTRY * menuEntry,
                                const char * command,
                                Print * display);

// Read data from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   flashAddress: Address of the first SPI flash byte to read
//   dataBuffer: Buffer to receive the SPI flash data
//   length: Number of bytes to read from the SPI flash
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the read was successful and false upon error
bool r4aSpiFlashRead(const R4A_SPI_FLASH * spiFlash,
                     uint32_t flashAddress,
                     uint8_t * dataBuffer,
                     size_t length,
                     Print * display);

// Read the block protection register from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   dataBuffer: Buffer to receive the block protection
//   length: Number of bytes to read from the SPI flash (1 - 18)
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the read was successful and false upon error
bool r4aSpiFlashBlockProtectionStatus(const R4A_SPI_FLASH * spiFlash,
                                      uint8_t * dataBuffer,
                                      size_t length,
                                      Print * display);

// Read the discovery parameters from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   flashAddress: Address of the first SPI flash byte to read
//   dataBuffer: Buffer to receive the SPI flash data
//   length: Number of bytes to read from the SPI flash
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the read ID was successful and false upon error
bool r4aSpiFlashReadDiscoveryParameters(const R4A_SPI_FLASH * spiFlash,
                                        uint32_t flashAddress,
                                        uint8_t * dataBuffer,
                                        size_t length,
                                        Print * display);

// Read the ID from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   dataBuffer: Buffer to receive the SPI flash data
//   length: Number of bytes to read from the SPI flash
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the read ID was successful and false upon error
bool r4aSpiFlashReadId9e(const R4A_SPI_FLASH * spiFlash,
                         uint8_t * dataBuffer,
                         size_t length,
                         Print * display);

// Read the ID from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   dataBuffer: Buffer to receive the SPI flash data of R4A_SPI_FLASH_9F_ID_BYTES
//   length: Number of bytes to read from the SPI flash
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the read ID was successful and false upon error
bool r4aSpiFlashReadId9f(const R4A_SPI_FLASH * spiFlash,
                         uint8_t dataBuffer,
                         Print * display);

// Get the maximum read length
size_t r4aSpiFlashReadMaxLength(const R4A_SPI_FLASH * spiFlash);

// Read status register from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   status: Buffer to receive the SPI flash status register
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the read was successful and false upon error
bool r4aSpiFlashReadStatusRegister(const R4A_SPI_FLASH * spiFlash,
                                   uint8_t * status,
                                   Print * display);

// Write data to the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   flashAddress: Address of the first SPI flash byte to write
//   writeBuffer: Buffer containing data to write
//   length: Number of bytes to write to the SPI flash
//   status: Address to receive the final write status
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write was successful and false upon error
bool r4aSpiFlashWrite(const R4A_SPI_FLASH * spiFlash,
                      uint32_t flashAddress,
                      uint8_t * writeBuffer,
                      size_t length,
                      uint8_t * status,
                      Print * display);

// Disable write operations
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write disable was successful and false upon error
bool r4aSpiFlashWriteDisable(const R4A_SPI_FLASH * spiFlash,
                             Print * display);

// Write status operations
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   status: New status value
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write status was successful and false upon error
bool r4aSpiFlashWriteStatus(const R4A_SPI_FLASH * spiFlash,
                            uint8_t status,
                            Print * display);

//****************************************
// SPI Flash Server API
//****************************************

#define CMD_READ_COMMAND            0
#define CMD_READ_DATA               1
#define CMD_WRITE_DATA              2
#define CMD_WRITE_SUCCESS           3
#define CMD_ERASE_CHIP              4
#define CMD_ERASE_SUCCESS           5
#define CMD_BLOCK_WRITE_ENABLE      6
#define CMD_BLOCK_ENABLE_SUCCESS    7

typedef struct _R4A_SPI_FLASH_COMMAND
{
    uint32_t _flashAddress;     // Flash address
    uint16_t _lengthInBytes;    // Number of bytes to read or write
    uint8_t _command;           // See CMD_* above
} R4A_SPI_FLASH_COMMAND;

// Initialize the SPI NOR Flash server
// Inputs:
//   ipAddress: IP address of the server
//   port: Server port number
// Outputs:
//   Returns true following successful server initialization and false
//   upon failure.
bool r4aSpiFlashServerBegin(IPAddress ipAddress, uint16_t port);

// Done with the SPI NOR Flash server
void r4aSpiFlashServerEnd();

// Update the server state
// Inputs:
//   connected: Set to true when connected to the network and false when
//   not connected
void r4aSpiFlashServerUpdate(bool connected);

//****************************************
// Support API
//****************************************

// Get the parameter
// Inputs:
//   parameter: Address of address of a zero terminated string of characters
// Outputs:
//   Returns the address of the next parameter
uint8_t * r4aSupportGetParameter(uint8_t ** parameter);

// Remove white space before a parameter
// Inputs:
//   parameter: Address of a zero terminated string of characters containing
//              leading white space and a parameter
// Outputs:
//   Returns the address of the parameter
uint8_t * r4aSupportRemoveWhiteSpace(uint8_t * parameter);

// Trim white space at the end of the parameter
// Inputs:
//   parameter: Address of a zero terminated string of characters containing
//              leading white space and a parameter
// Outputs:
//   Returns the address of the parameter
void r4aSupportTrimWhiteSpace(uint8_t * parameter);

//****************************************
// Telnet Client API
//****************************************

// Finish creating the network client
// Inputs:
//   parameter: Buffer to receive the address of an object allocated by
//              this routine
//   client: Address of a NetworkClient object
// Outputs:
//   Returns true if the routine was successful and false upon failure.
typedef bool (* R4A_TELNET_CONTEXT_CREATE)(void ** parameter,
                                           NetworkClient * client);

// Clean up after the parameter object returned by r4aTelnetClientBegin
// Inputs:
//   parameter: Buffer containing the address of a data structure allocated
//              by R4A_TELNET_CONTEXT_CREATE
typedef void (* R4A_TELNET_CONTEXT_DELETE)(void ** parameter);

// Process input characters from the telnet client
// Inputs:
//   parameter: Address of object allocated by R4A_TELNET_CONTEXT_CREATE
//   client: Address of a NetworkClient object
// Outputs:
//   Returns true if the client is done (requests exit)
typedef bool (* R4A_TELNET_CLIENT_PROCESS_INPUT)(void * parameter,
                                                 NetworkClient * client);

// Telnet client
class R4A_TELNET_CLIENT
{
  private:

    NetworkClient _client;
    String _command;
    R4A_TELNET_CONTEXT_CREATE _contextCreate;
    void * _contextData;
    R4A_TELNET_CONTEXT_DELETE _contextDelete;
    R4A_TELNET_CLIENT_PROCESS_INPUT _processInput;

  public:

    // Constructor: Initialize the telnet client object
    // Inputs:
    //   client: Address of a NetworkClient object
    //   processInput: Address of a function to process input characters
    //   contextCreate: Address of a function to create the telnet context
    //   contextDelete: Address of a function to delete the telnet context
    R4A_TELNET_CLIENT(NetworkClient client,
                      R4A_TELNET_CLIENT_PROCESS_INPUT processInput,
                      R4A_TELNET_CONTEXT_CREATE contextCreate = nullptr,
                      R4A_TELNET_CONTEXT_DELETE contextDelete = nullptr);

    // Destructor: Cleanup the things allocated in the constructor
    ~R4A_TELNET_CLIENT();

    // Break the client connection
    void disconnect();

    // Determine if the client is connected
    // Outputs:
    //   Returns true if the client is connected
    bool isConnected();

    // Process the incoming telnet client data
    // Outputs:
    //   Returns true when the client wants to exit, false to continue
    //   processing input.
    bool processInput();

    // Get the IP address
    // Outputs:
    //   Returns the IP address of the remote telnet client.
    IPAddress remoteIP();

    // Get the port number
    // Outputs:
    //   Returns the port number used by the remote telnet client.
    uint16_t remotePort();

    // Write to all clients
    // Inputs:
    //   data: Data byte to write to the clients
    // Outputs:
    //   Returns the number of bytes written
    size_t write(uint8_t data);

    // Write to all clients
    // Inputs:
    //   data: Address of the data to write to the clients
    //   length: Number of data bytes to write
    // Outputs:
    //   Returns the number of bytes written
    size_t write(const uint8_t * data, size_t length);
};

//*********************************************************************
// Data associated with the telnet connection
typedef struct _R4A_TELNET_CONTEXT
{
    // Public
    String _command; // User command received via telnet
    bool _displayOptions;
    bool _echo;
    R4A_MENU _menu;  // Strcture describing the menu system
} R4A_TELNET_CONTEXT;

// Initialize the R4A_TELNET_CONTEXT data structure
// Inputs:
//   context: Address of an R4A_TELNET_CONTEXT data structure
//   menuTable: Address of table containing the menu descriptions, the
//              main menu must be the first entry in the table.
//   menuEntries: Number of entries in the menu table
//   blankLineBeforePreMenu: Display a blank line before the preMenu
//   blankLineBeforeMenuHeader: Display a blank line before the menu header
//   blankLineAfterMenuHeader: Display a blank line after the menu header
//   alignCommands: Align the commands
//   blankLineAfterMenu: Display a blank line after the menu
void r4aTelnetContextBegin(R4A_TELNET_CONTEXT * context,
                           int menuTableEntries,
                           bool displayOptions = false,
                           bool echo = false,
                           bool blankLineBeforePreMenu = true,
                           bool blankLineBeforeMenuHeader = true,
                           bool blankLineAfterMenuHeader = false,
                           bool alignCommands = true,
                           bool blankLineAfterMenu = false);

// Finish creating the network client
// Inputs:
//   contextAddr: Buffer to receive the address of an R4A_TELNET_CONTEXT
//                data structure allocated by this routine
//   client: Address of a NetworkClient object
//   menuTable: Address of table containing the menu descriptions, the
//              main menu must be the first entry in the table.
//   menuEntries: Number of entries in the menu table
//   displayOptions: Display the telnet header options
//   echo: Echo the input characters
//   blankLineBeforePreMenu: Display a blank line before the preMenu
//   blankLineBeforeMenuHeader: Display a blank line before the menu header
//   blankLineAfterMenuHeader: Display a blank line after the menu header
//   alignCommands: Align the commands
//   blankLineAfterMenu: Display a blank line after the menu
// Outputs:
//   Returns true if the routine was successful and false upon failure.
bool r4aTelnetContextCreate(void ** contextAddr,
                            NetworkClient * client,
                            const R4A_MENU_TABLE * menuTable,
                            int menuTableEntries,
                            bool displayOptions = false,
                            bool echo = false,
                            bool blankLineBeforePreMenu = true,
                            bool blankLineBeforeMenuHeader = true,
                            bool blankLineAfterMenuHeader = false,
                            bool alignCommands = true,
                            bool blankLineAfterMenu = false);

// Clean up after the parameter object returned by r4aTelnetContextCreate
// Inputs:
//   contextAddr: Buffer to containing the address of an R4A_TELNET_CONTEXT
//                data structure allocated by r4aTelnetContextCreate
void r4aTelnetContextDelete(void ** contextAddr);

// Process input from the telnet client
// Inputs:
//   contextData: Address of an R4A_TELNET_CONTEXT data structure allocated
//                by r4aTelnetContextCreate
//   client: Address of a NetworkClient object
// Outputs:
//   Returns true if the client is done (requests exit)
bool r4aTelnetContextProcessInput(void * contextData,
                                  NetworkClient * client);

//****************************************
// Telnet Server API
//****************************************

class R4A_TELNET_SERVER : public Print
{
  private:

    int _activeClients;
    R4A_TELNET_CLIENT ** _clients;
    R4A_TELNET_CONTEXT_CREATE _contextCreate;
    R4A_TELNET_CONTEXT_DELETE _contextDelete;
    IPAddress _ipAddress;
    const int _maxClients;
    uint16_t _port;
    R4A_TELNET_CLIENT_PROCESS_INPUT _processInput;
    NetworkServer * _server;

    // Close the client specified by index i
    // Inputs:
    //   i: Index into the _clients list
    void closeClient(uint16_t i);

    // Create a new telnet client
    void newClient();

  public:

    // Constructor: Create the telnet server object
    // Inputs:
    //   maxClients: Specify the maximum number of clients supported by
    //               the telnet server
    //   processInput: Address of a function to process input characters
    //   contextCreate: Address of a function to create the telnet context
    //   contextDelete: Address of a function to delete the telnet context
    R4A_TELNET_SERVER(uint16_t maxClients,
                      R4A_TELNET_CLIENT_PROCESS_INPUT processInput,
                      R4A_TELNET_CONTEXT_CREATE contextCreate = nullptr,
                      R4A_TELNET_CONTEXT_DELETE contextDelete = nullptr);

    // Destructor: Cleanup the things allocated in the constructor
    ~R4A_TELNET_SERVER();

    // Initialize the telnet server
    // Inputs:
    //   ipAddress: IP Address of the server
    //   port: The port on the server used for telnet client connections
    // Outputs:
    //   Returns true following successful server initialization and false
    //   upon failure.
    bool begin(IPAddress ipAddress, uint16_t port);

    // Restore state to just after constructor execution
    void end();

    // Determine if the telnet server has any clients
    // Outputs:
    //   Returns true if one or more clients are connected.
    bool hasClient();

    // Get the IP address
    IPAddress ipAddress(void);

    // List the clients
    // Inputs:
    //   display: Device used for output
    void listClients(Print * display = &Serial);

    // Get the port number
    uint16_t port();

    // Display the server information
    // Inputs:
    //   display: Device used for output
    void serverInfo(Print * display = &Serial);

    // Update the server state
    // Inputs:
    //   connected: True when the network is connected and false upon
    //              network failure
    void update(bool connected);

    // Write to all clients
    // Inputs:
    //   data: Data byte to write to the clients
    // Outputs:
    //   Returns the number of bytes written
    size_t write(uint8_t data);

    // Write to all clients
    // Inputs:
    //   data: Address of the data to write to the clients
    //   length: Number of data bytes to write
    // Outputs:
    //   Returns the number of bytes written
    size_t write(const uint8_t * data, size_t length);
};

//****************************************
// Time API
//****************************************

typedef uint32_t R4A_TIME_USEC_t;

// Compute the average of a list of microsecond entries
// Inputs:
//   list: List of times in microseconds
//   entries: Number of entries in the list
//   maximumUsec: Address of a buffer to receive the maximum value
//   minimumUsec: Address of a buffer to receive the minimum value
// Outputs:
//   Returns the average value in microseconds
R4A_TIME_USEC_t r4aTimeComputeAverageUsec(R4A_TIME_USEC_t * list,
                                          uint32_t entries,
                                          R4A_TIME_USEC_t * maximumUsec,
                                          R4A_TIME_USEC_t * minimumUsec);

// Compute the standard deviation of a list of microsecond entries
// Inputs:
//   list: List of times in microseconds
//   entries: Number of entries in the list
//   averageUsec: Average number of microseconds
// Outputs:
//   Returns the standard deviation value in microseconds
R4A_TIME_USEC_t r4aTimeComputeStdDevUsec(R4A_TIME_USEC_t * list,
                                         uint32_t entries,
                                         R4A_TIME_USEC_t averageUsec);

// Format the time
// Inputs:
//   buffer: Address of a buffer to receive the time string
//   uSec: Microsecond value to convert into a string
void r4aTimeFormatLoopTime(char * buffer, R4A_TIME_USEC_t uSec);

// Display loop times
// Inputs:
//   display: Device used for output
//   list: List of times in microseconds
//   entries: Number of entries in the list
//   text: Zero terminated string describing the time values
void r4aTimeDisplayLoopTimes(Print * display,
                             R4A_TIME_USEC_t * list,
                             uint32_t entries,
                             const char * text);

//****************************************
// Time Zone API
//****************************************

extern int8_t r4aTimeZoneHours;
extern int8_t r4aTimeZoneMinutes;
extern int8_t r4aTimeZoneSeconds;

//****************************************
// Waypoints API
//****************************************

typedef struct _R4A_LAT_LONG_POINT
{
    double latitude;
    double longitude;
    double hpa;
    uint8_t siv;
} R4A_LAT_LONG_POINT;

typedef struct _R4A_LAT_LONG_POINT_PAIR
{
    R4A_LAT_LONG_POINT current;
    R4A_LAT_LONG_POINT previous;
} R4A_LAT_LONG_POINT_PAIR;

typedef struct _R4A_HEADING
{
    // Inputs
    R4A_LAT_LONG_POINT_PAIR * location;

    // Outputs
    R4A_LAT_LONG_POINT delta;

    char eastWest;
    int eastWestFeet;
    double eastWestInches;
    double eastWestInchesTotal;

    char northSouth;
    int northSouthFeet;
    double northSouthInches;
    double northSouthInchesTotal;

    int feet;
    double inches;
    double inchesTotal;
    double radians;
    double degrees;
} R4A_HEADING;

// Determine the central angle between two points on a sphere
// See https://en.wikipedia.org/wiki/Haversine_formula
// Inputs:
//   point: Address of an R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns the central angle between the two points on the sphere
double r4aCentralAngle(R4A_LAT_LONG_POINT_PAIR * point);

// Compute the heading
// Inputs:
//   heading: Address of a R4A_HEADING object
void r4aComputeHeading(R4A_HEADING * heading);

// Display the heading
// Inputs:
//   heading: Address of a R4A_HEADING object
//   text: Zero terminated line of text to display
//   display: Address of a Print object to display the output
void r4aDisplayHeading(R4A_HEADING * heading,
                       const char * text,
                       Print * display = &Serial);

// Determine the ellipsoidal flattening
// See https://en.wikipedia.org/wiki/Flattening
// Inputs:
//   longRadius: The longer of the two radii of the ellipsoid
//   shortRadius: The shorter of the two radii of the ellipsoid
// Outputs:
//   Returns the flattening value
double r4aFlatening(double longRadius, double shortRadius);

// Determine the haversine distance between two points on a sphere
// See https://en.wikipedia.org/wiki/Haversine_formula
// Inputs:
//   radius: Radius of the sphere
//   point: Address of an R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns the great circle distance between the two points on the sphere
double r4aHaversineDistance(double radius, R4A_LAT_LONG_POINT_PAIR * point);

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
                          R4A_LAT_LONG_POINT_PAIR * point);

// Determine the haversine distance between two points on a earth
// See https://en.wikipedia.org/wiki/Haversine_formula
// Inputs:
//   radius: Radius of the sphere
//   point: Address of an R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns the great circle distance between the two points on the earth
//   in kilometers
double r4aWaypointHaversineDistance(R4A_LAT_LONG_POINT_PAIR * point);

// Determine the Lambert distance between two points on a earth
// See https://www.calculator.net/distance-calculator.html
// Inputs:
//   point: Address of an R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns the ellipsoidal distance between the two points on the earth
//   in kilometers
double r4aWaypointLambertDistance(R4A_LAT_LONG_POINT_PAIR * point);

// Determine if the waypoint was reached
// Inputs:
//   point: Address of R4A_LAT_LONG_POINT_PAIR object
// Outputs:
//   Returns true if the position is close enough to the waypoint
bool r4aWaypointReached(R4A_LAT_LONG_POINT_PAIR * point);

#endif  // __R4A_ROBOT_H__
