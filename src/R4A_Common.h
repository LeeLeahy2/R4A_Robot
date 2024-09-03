/**********************************************************************
  R4A_Common.h

  Declare the common support
**********************************************************************/

#ifndef __R4A_COMMON_H__
#define __R4A_COMMON_H__

#include <Arduino.h>            // Built-in
#include <WiFi.h>               // Built-in

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
// Stricmp API
//****************************************

// Compare two strings ignoring case
// Inputs:
//   str1: Address of a zero terminated string of characters
//   str2: Address of a zero terminated string of characters
// Outputs:
//   Returns the delta value of the last comparison (str1[x] - str2[x])
int r4aEsp32Stricmp(const char *str1, const char *str2);

#endif  // __R4A_COMMON_H__
