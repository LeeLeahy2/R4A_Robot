/**********************************************************************
  LED.cpp

  Robots-For-All (R4A)
  Multicolor LED support
**********************************************************************/

#include "R4A_Robot.h"

//****************************************
// Constants
//****************************************

#define R4A_LED_BLUE        0
#define R4A_LED_GREEN       1
#define R4A_LED_RED         2
#define R4A_LED_WHITE       3

#define R4A_LED_RESET       (5 * 6) // Zero bytes to cause reset
#define R4A_LED_ONES        0       // One bytes to prevent reset

extern const uint8_t r4aLEDIntensityTable[]; // Color intensity bits to bytes

//****************************************
// Globals
//****************************************

uint32_t * r4aLEDColor;
volatile bool r4aLEDColorWritten;
uint8_t *  r4aLEDFourColorsBitmap;
uint8_t r4aLEDIntensity = 255;
uint8_t r4aLEDs;
uint8_t *  r4aLEDTxDmaBuffer;

//*********************************************************************
// The WS2812 LED specification uses the following bit definitions at
// approximately 800 KHz:
//
//            WS2812 Specification                              SPI
//         __________                               ________
//  0:  __|          |_____________________|       |        |________________|
//         0.35 uSec          0.8 uSec              0.5 uSec      0.75 uSec
//         +/- 150 uSec       +/- 150 uSec
//         ____________________                     ________________
//  1:  __|                    |_____________|     |                |________|
//               0.7 uSec        0.6 uSec               0.75 uSec    0.5 uSec
//               +/- 150 uSec    +/- 150 uSec
//
//
// The SK6812RGBW LED specification uses the following bit definitions
// at approximately 800 KHz.
//
//         ________
//  0:  __|        |________________________|
//         0.3 uSec           0.9 uSec
//         +/- 150 uSec       +/- 150 uSec
//
//         ________________
//  1:  __|                |________________|
//          0.6 uSec          0.6 uSec
//         +/- 150 uSec       +/- 150 uSec
//
//
// This data stream is approximated using a SPI data stream clocked at
// 4 MHz.
//
// The color data is sent most significant bit first in the order of green
// then red then blue.
//
//                          +-----------+-----------+-----------+
//      WS2812 LED  <-----  |7  Green  0|7   Red   0|7  Blue   0|
//                          +-----------+-----------+-----------+
//
//                              +-----------+-----------+-----------+-----------+
//      SK6812RGBW LED  <-----  |7   Red   0|7  Green  0|7  Blue   0|7  White  0|
//                              +-----------+-----------+-----------+-----------+

//*********************************************************************
// Set the WS2812 LED colors
void r4aLEDSetColorRgb(uint8_t ledNumber, uint32_t color)
{
    // Red Bits:   23 - 16
    // Green Bits: 15 -  8
    // Blue Bits:   7 -  0

    // Verify the LED number
    if (ledNumber < r4aLEDs)
    {
        // Indicate that this LED uses 3 colors
        r4aLEDFourColorsBitmap[ledNumber >> 3] &= ~(1 << (ledNumber & 7));

        // Set the LED color
        r4aLEDColor[ledNumber] = color;
        r4aLEDColorWritten = true;
    }
    else
        Serial.printf("ERROR: ledNumber needs to be in the range of 0 - %d!",
                      r4aLEDs);
}

//*********************************************************************
// Set the WS2812 LED colors
void r4aLEDSetColorRgb(uint8_t ledNumber,
                       uint8_t red,
                       uint8_t green,
                       uint8_t blue)
{
    uint32_t color;

    // Construct the color value
    color = (((uint32_t)red) << (R4A_LED_RED << 3))
          | (((uint32_t)green) << (R4A_LED_GREEN << 3))
          | (((uint32_t)blue) << (R4A_LED_BLUE << 3));

    // Verify the LED number
    if (ledNumber < r4aLEDs)
    {
        // Indicate that this LED uses 3 colors
        r4aLEDFourColorsBitmap[ledNumber >> 3] &= ~(1 << (ledNumber & 7));

        // Set the LED color
        r4aLEDColor[ledNumber] = color;
        r4aLEDColorWritten = true;
    }
    else
        Serial.printf("ERROR: ledNumber needs to be in the range of 0 - %d!",
                      r4aLEDs);
}

//*********************************************************************
// Set the SK6812RGBW LED colors
void r4aLEDSetColorWrgb(uint8_t ledNumber, uint32_t color)
{
    uint8_t index;

    // White Bits: 32 - 24
    // Red Bits:   23 - 16
    // Green Bits: 15 -  8
    // Blue Bits:   7 -  0

    // Verify the LED number
    if (ledNumber < r4aLEDs)
    {
        // Indicate that this LED uses 4 colors
        r4aLEDFourColorsBitmap[ledNumber >> 3] |= 1 << (ledNumber & 7);

        // Set the LED color
        r4aLEDColor[ledNumber] = color;
        r4aLEDColorWritten = true;
    }
    else
        Serial.printf("ERROR: ledNumber needs to be in the range of 0 - %d!",
                      r4aLEDs);
}

//*********************************************************************
// Set the SK6812RGBW LED colors
void r4aLEDSetColorWrgb(uint8_t ledNumber,
                        uint8_t white,
                        uint8_t red,
                        uint8_t green,
                        uint8_t blue)
{
    uint32_t color;
    uint8_t index;

    // Construct the color value
    color = (((uint32_t)red) << (R4A_LED_RED << 3))
          | (((uint32_t)green) << (R4A_LED_GREEN << 3))
          | (((uint32_t)blue) << (R4A_LED_BLUE << 3))
          | (((uint32_t)white) << (R4A_LED_WHITE << 3));

    // Verify the LED number
    if (ledNumber < r4aLEDs)
    {
        // Indicate that this LED uses 4 colors
        r4aLEDFourColorsBitmap[ledNumber >> 3] |= 1 << (ledNumber & 7);

        // Set the LED color
        r4aLEDColor[ledNumber] = color;
        r4aLEDColorWritten = true;
    }
    else
        Serial.printf("ERROR: ledNumber needs to be in the range of 0 - %d!",
                      r4aLEDs);
}

//*********************************************************************
// Set the LED intensity
void r4aLEDSetIntensity(uint8_t intensity)
{
    // Change the intensity value
    r4aLEDIntensity = intensity;
    r4aLEDColorWritten = true;
}

//*********************************************************************
// Initialize the LEDs
bool r4aLEDSetup(uint8_t spiNumber,
                 uint8_t pinMOSI,
                 uint32_t clockHz,
                 uint8_t numberOfLEDs)
{
    int ledBytes;
    int length;

    do
    {
        // Determine if the SPI controller object was allocated
        if (!r4aSpi)
        {
            Serial.println("ERROR: Failed to allocate r4aSpi!");
            break;
        }

        // Remember the number of LEDs
        r4aLEDs = numberOfLEDs;

        // Allocate the TX DMA buffer
        // This buffer needs to be in static RAM to allow DMA access
        // Assume all LEDs support 4 colors, 8-bits per color and 5 SPI
        // bits / color bit packed in 8 bits per byte
        ledBytes = numberOfLEDs * 4 * 5;
        length = R4A_LED_RESET + ledBytes + R4A_LED_ONES;
        r4aLEDTxDmaBuffer = r4aSpi->allocateDmaBuffer(length);
        if (!r4aLEDTxDmaBuffer)
        {
            Serial.println("ERROR: Failed to allocate r4aLEDTxDmaBuffer!");
            break;
        }
        memset(r4aLEDTxDmaBuffer, 0, length);

        // Allocate the color array, assume four 8-bit colors per LED
        length = numberOfLEDs << 2;
        r4aLEDColor = (uint32_t *)malloc(length);
        if (!r4aLEDColor)
        {
            Serial.println("ERROR: Failed to allocate r4aLEDColor!");
            break;
        }
        memset(r4aLEDColor, 0, length);

        // Allocate the 4 color bitmap
        length = (numberOfLEDs + 7) >> 3;
        r4aLEDFourColorsBitmap = (uint8_t *)malloc(length);
        if (!r4aLEDFourColorsBitmap)
        {
            Serial.println("ERROR: Failed to allocate r4aLEDFourColorsBitmap!");
            break;
        }

        // Assume all LEDs are 4 color
        memset(r4aLEDFourColorsBitmap, 0xff, length);

        // Initialize the SPI controller
        if (r4aSpi->begin(spiNumber, pinMOSI, clockHz))
        {
            // Turn off the LEDs
            r4aLEDColorWritten = true;
            r4aLEDUpdate(true);
            return true;
        }
    } while (0);

    // Free the allocated memory
    if (r4aLEDColor)
    {
        free(r4aLEDColor);
        r4aLEDColor = nullptr;
    }
    if (r4aLEDFourColorsBitmap)
    {
        free(r4aLEDFourColorsBitmap);
        r4aLEDFourColorsBitmap = nullptr;
    }
    if (r4aLEDTxDmaBuffer)
    {
        free(r4aLEDTxDmaBuffer);
        r4aLEDTxDmaBuffer = nullptr;
    }
    return false;
}

//*********************************************************************
// Turn off the LEDs
void r4aLEDsOff()
{
    // Set the LED colors
    for (uint8_t led = 0; led < r4aLEDs; led++)
        if (r4aLEDFourColorsBitmap[led >> 7] & (1 << (led & 7)))
            r4aLEDSetColorWrgb(led, 0);
        else
            r4aLEDSetColorRgb(led, 0);
}

//*********************************************************************
// Update the colors on the LEDs
void r4aLEDUpdate(bool updateRequest)
{
    uint32_t color;
    uint8_t * data;
    int index;
    int intensity;
    static int length;

    // Check for a color change
    if (r4aLEDColorWritten)
    {
        r4aLEDColorWritten = false;
        updateRequest = true;

        // Add the reset sequence
        data = r4aLEDTxDmaBuffer;
        if (R4A_LED_RESET)
        {
            memset(data, 0, R4A_LED_RESET);
            data += R4A_LED_RESET;
        }

        // Walk the array of LEDs
        for (int led = 0; led < r4aLEDs; led++)
        {
            // Copy the color array
            color = r4aLEDColor[led];

            // Determine the LED type
            if (r4aLEDFourColorsBitmap[led >> 3] & (1 << (led & 7)))
            {
                //                              +-----------+-----------+-----------+-----------+
                //      SK6812RGBW LED  <-----  |7   Red   0|7  Green  0|7  Blue   0|7  White  0|
                //                              +-----------+-----------+-----------+-----------+
                //
                // White Bits: 32 - 24
                // Red Bits:   23 - 16
                // Green Bits: 15 -  8
                // Blue Bits:   7 -  0

                // Red
                intensity = (color >> 16) & 0xff;
                intensity = (intensity * r4aLEDIntensity) / 255;
                index = (intensity << 2) + intensity;
                memcpy(data, &r4aLEDIntensityTable[index], 5);
                data += 5;

                // Green
                intensity = (color >> 8) & 0xff;
                intensity = (intensity * r4aLEDIntensity) / 255;
                index = (intensity << 2) + intensity;
                memcpy(data, &r4aLEDIntensityTable[index], 5);
                data += 5;

                // Blue
                intensity = color & 0xff;
                intensity = (intensity * r4aLEDIntensity) / 255;
                index = (intensity << 2) + intensity;
                memcpy(data, &r4aLEDIntensityTable[index], 5);
                data += 5;

                // White
                intensity = (color >> 24) & 0xff;
                intensity = (intensity * r4aLEDIntensity) / 255;
                index = (intensity << 2) + intensity;
                memcpy(data, &r4aLEDIntensityTable[index], 5);
                data += 5;
            }
            else
            {
                //                          +-----------+-----------+-----------+
                //      WS2812 LED  <-----  |7  Green  0|7   Red   0|7  Blue   0|
                //                          +-----------+-----------+-----------+
                //
                // Red Bits:   23 - 16
                // Green Bits: 15 -  8
                // Blue Bits:   7 -  0

                // Green
                intensity = (color >> 8) & 0xff;
                intensity = (intensity * r4aLEDIntensity) / 255;
                index = (intensity << 2) + intensity;
                memcpy(data, &r4aLEDIntensityTable[index], 5);
                data += 5;

                // Red
                intensity = (color >> 16) & 0xff;
                intensity = (intensity * r4aLEDIntensity) / 255;
                index = (intensity << 2) + intensity;
                memcpy(data, &r4aLEDIntensityTable[index], 5);
                data += 5;

                // Blue
                intensity = color & 0xff;
                intensity = (intensity * r4aLEDIntensity) / 255;
                index = (intensity << 2) + intensity;
                memcpy(data, &r4aLEDIntensityTable[index], 5);
                data += 5;
            }
        }

        // Set the ones
        if (R4A_LED_ONES)
        {
            memset(data, 0xff, R4A_LED_ONES);
            data += R4A_LED_ONES;
        }

        // Determine the amount of data to send to the LEDs
        length = data - r4aLEDTxDmaBuffer;
    }

    // Output the color data to the LEDs
    if (updateRequest)
        r4aSpi->transfer(r4aLEDTxDmaBuffer, nullptr, length);
}