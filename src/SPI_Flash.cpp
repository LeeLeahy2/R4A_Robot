/**********************************************************************
  SPI_Flash.cpp

  Robots-For-All (R4A)

  SPI NOR Flash support

  Tested with the Silicon Storage Technology SST26VF032BA-104I/MF found
  in the Alchitry Pt V2 FPGA board.

                    ESP32                           SPI Flash
            ---------------------.               .-------------.
                 Chip Select Bar |-------------->| S#          |
                                 |               |             |
                            MOSI |-------------->| DQ0         |
                                 |               |             |
                            MISO |-------------->| DQ1         |
                                 |               |             |
                           Clock |-------------->| C           |
                                 |               |             |
               Write Protect Bar |-------------->| W#          |
                                 |               |             |
                        Hold Bar |-------------->| HOLD#       |
            ---------------------'               '-------------'

    Chip Select Bar: When high enters standby mode and causes DQ1 to
        tristate.  When low enables reads and writes to the SPI flash.

    MOSI: Data sent from the CPU to the SPI flash device.

    MISO: Data sent from the SPI flash device to the CPU.

    Clock: Provides timing for the SPI data lines.  Provide data on the
        falling clock edge and latch data on the rising clock edge.

    Write Protect: Drive this pin low when not programming the SPI flash.

    Hold: Always drive this pin high.  Pauses communications while the
        device is selected, DQ1 tristates, Clock and MOSI are don't cares.

    Operations:

              Addr  Dummy
        Code  Bytes Cycles  Data Bytes      Operation
         01     0     0     1               Write status register
         02     3     0     1 to 256        Page program
         04     0     0     0               Write disable
         05     0     0     1 or more       Read status register
         06     0     0     0               Write enable
         0b     3     8     1 or more       Read data
         20     3     0     0               Subsector erase
         5a     3     8     1 or more       Read serial flash discovery parameter
         9e     0     0     1 to 20         Read ID
         c7     0     0     0               Bulk erase
         d8     3     0     0               Sector erase

**********************************************************************/

#include "R4A_Robot.h"

//****************************************
// Constants
//****************************************

#define CMD_CHIP_ERASE          0xc7
#define CMD_CHIP_ERASE_LENGTH   1           // Bulk erase

#define CMD_ERASE_BLOCK         0xd8        // 8K, 32K or 65K bytes
#define CMD_ERASE_BLOCK_LENGTH  (1 + 3)     // Erase block command, flash address

#define CMD_ERASE_SECTOR        0x20        // 4k bytes
#define CMD_ERASE_SECTOR_LENGTH (1 + 3)     // Erase sector command, flash address

#define CMD_LOCK_READ           0xe8
#define CMD_LOCK_READ_LENGTH    (1 + 3)

#define CMD_LOCK_WRITE          0xe5
#define CMD_LOCK_WRITE_LENGTH   (1 + 3)

#define CMD_READ                0x0b
#define CMD_READ_LENGTH         (1 + 3 + 1) // Read command, flash address, dummy cycles

#define CMD_READ_BLOCK_PROTECT          0x72
#define CMD_READ_BLOCK_PROTECT_DATA     10
#define CMD_READ_BLOCK_PROTECT_LENGTH   1   // Read block protection

#define CMD_READ_DISCOVERY      0x5a
#define CMD_READ_DISCOVERY_LENGTH   (1 + 3 + 1) // Read discovery command, flash address, dummy cycles

#define CMD_READ_ID_9E          0x9e
#define CMD_READ_ID_9E_LENGTH   1           // Read ID command

#define CMD_READ_ID_9F          0x9f
#define CMD_READ_ID_9F_LENGTH   1           // Read ID command

#define CMD_READ_STATUS         0x05
#define CMD_READ_STATUS_LENGTH  1           // Read status command

#define CMD_WRITE_BLOCK_PROTECT 0x42
#define CMD_WRITE_BLOCK_PROTECT_DATA    CMD_READ_BLOCK_PROTECT_DATA
#define CMD_WRITE_BLOCK_PROTECT_LENGTH  1   // Write block protection register

#define CMD_WRITE_DISABLE       0x04
#define CMD_WRITE_DISABLE_LENGTH    1

#define CMD_WRITE_ENABLE        0x06
#define CMD_WRITE_ENABLE_LENGTH    1

#define CMD_WRITE               0x02
#define CMD_WRITE_LENGTH        (1 + 3)     // Write command, flash address

#define CMD_WRITE_STATUS        0x01
#define CMD_WRITE_STATUS_LENGTH (1 + 1 + 1) // Write enable, write status command, new status

#define ERASE_SECTOR_SIZE       65536
#define ERASE_SUBSECTOR_SIZE    4096

#define EXTRA_BYTES     \
      ((CMD_CHIP_ERASE_LENGTH > CMD_WRITE_LENGTH) ? CMD_CHIP_ERASE_LENGTH           \
    : ((CMD_ERASE_BLOCK_LENGTH > CMD_WRITE_LENGTH) ? CMD_ERASE_BLOCK_LENGTH         \
    : ((CMD_ERASE_SECTOR_LENGTH > CMD_WRITE_LENGTH) ? CMD_ERASE_SECTOR_LENGTH       \
    : ((CMD_READ_LENGTH > CMD_WRITE_LENGTH) ? CMD_READ_LENGTH                       \
    : ((CMD_READ_ID_LENGTH > CMD_WRITE_LENGTH) ? CMD_READ_ID_LENGTH                 \
    : ((CMD_READ_STATUS_LENGTH > CMD_WRITE_LENGTH) ? CMD_READ_STATUS_LENGTH         \
    : CMD_WRITE_LENGTH))))))

#define MIN_BUFFER_LENGTH       8

//*********************************************************************
// Initialize the connection to the SPI flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
bool r4aSpiFlashBegin(const R4A_SPI_FLASH * spiFlash)
{
    int8_t pin;

    // Disable access to the SPI flash
    pin = spiFlash->_flashChip._pinCS;
    if (pin >= 0)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, 1);
    }

    // Write protect the SPI flash
    pin = spiFlash->_pinWriteProtect;
    if (pin >= 0)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, 0);
    }

    // Don't pause the transactions
    pin = spiFlash->_pinHold;
    if (pin >= 0)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, 1);
    }
    return true;
}

//*********************************************************************
// Enable or disable write operations using the write protect pin
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   enable: Set true to enable write operations and false to protect the chip
static void r4aSpiFlashChipWriteEnable(const R4A_SPI_FLASH * spiFlash,
                                       bool enable)
{
    int8_t pin;

    // Use the pin to change write protection
    pin = spiFlash->_pinWriteProtect;
    if (pin >= 0)
        digitalWrite(pin, enable ? 1 : 0);

    // Use the write protect routine to change write protection
    else if (spiFlash->_writeEnablePinState)
        // Update the state of the write enable pin
        spiFlash->_writeEnablePinState(enable);
}

//*********************************************************************
// Enable write operations
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write disable was successful and false upon error
bool r4aSpiFlashWriteEnable(const R4A_SPI_FLASH * spiFlash,
                            Print * display)
{
    uint8_t buffer;
    bool success;

    // Build the erase command
    buffer = CMD_WRITE_ENABLE;

    // Perform the erase sector operation
    success = r4aSpiTransfer(&spiFlash->_flashChip,
                             &buffer,
                             nullptr,
                             CMD_WRITE_ENABLE_LENGTH,
                             display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash write disable failure!\r\n");
    }

    // Determine if the write enable was complete
    return success;
}

//*********************************************************************
// Read the block protection from the SPI NOR flash
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
                                      Print * display)
{
    uint8_t * buffer;
    bool success;

    // Allocate a temporary buffer
    buffer = (uint8_t *)r4aMalloc(length + CMD_READ_BLOCK_PROTECT_LENGTH,
                                  "SPI Flash read block protection buffer");
    if (buffer == nullptr)
    {
        if (display)
            display->printf("SPI Flash: Failed to allocate the buffer\r\n");
        return false;
    }

    // Build the read block protections command
    memset(buffer, 0, length + CMD_READ_BLOCK_PROTECT_LENGTH);
    buffer[0] = CMD_READ_BLOCK_PROTECT;

    // Perform the read block protections operation
    success = r4aSpiTransfer(&spiFlash->_flashChip,
                             buffer,
                             buffer,
                             CMD_READ_BLOCK_PROTECT_LENGTH + length,
                             display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash read failure!\r\n");
    }
    else
        // Move the block protection data into the buffer
        memcpy(dataBuffer, &buffer[CMD_READ_BLOCK_PROTECT_LENGTH], length);

    // Free the temporary buffer
    r4aFree(buffer, "SPI Flash read block protection buffer");

    // Determine if the read block protection was complete
    return success;
}

//*********************************************************************
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
                                    Print * display)
{
    union
    {
        uint8_t u8[4];
        uint32_t u32;
    } address;
    int bit;
    int bitNumber;
    int length = CMD_WRITE_BLOCK_PROTECT_LENGTH + spiFlash->_blockProtectBytes;
    uint8_t buffer[length];
    int entry;
    int index;
    bool success;

    // Perform the necessary write operations
    address.u32 = flashAddress;
    success = true;
    do
    {
        // Get the block protect data
        success = r4aSpiFlashBlockProtectionStatus(spiFlash,
                                                   &buffer[CMD_WRITE_BLOCK_PROTECT_LENGTH],
                                                   spiFlash->_blockProtectBytes,
                                                   nullptr);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: Failed to read the block protection register!\r\n");
            break;
        }

        // Walk the list of protection bits
        for (entry = 0;
             spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
             entry++)
        {
            // Locate the bit to change
            if ((spiFlash->_blockProtect[entry]._flashAddress <= flashAddress)
                && (spiFlash->_blockProtect[entry + 1]._flashAddress > flashAddress))
            {
                bitNumber = spiFlash->_blockProtect[entry]._readProtectBit;
                index = bitNumber >> 3;
                bit = 1 << (bitNumber & 7);
                index += CMD_WRITE_BLOCK_PROTECT_LENGTH;
                break;
            }
        }
        if (spiFlash->_blockProtect[entry]._flashAddress >= spiFlash->_flashBytes)
        {
            if (display)
                display->printf("ERROR: Bad flash address, needs to be in the range (0 - 0x%08x)\r\n",
                                spiFlash->_flashBytes);
            success = false;
            break;
        }

        // Skip if the read protection is already in correct state
        if ((bitNumber < 0) || ((buffer[index] & bit) == (enable ? 0 : 1)))
            break;

        do
        {
            // Enable writes with the WP# pin
            r4aSpiFlashChipWriteEnable(spiFlash, true);

            // Issue the write enable command
            success = r4aSpiFlashWriteEnable(spiFlash, display);
            if (success == false)
            {
                // Display the error
                if (display)
                    display->printf("ERROR: SPI Flash write enable command failure!\r\n");
                break;
            }

            // Update the bit
            if (enable)
                buffer[index] &= ~bit;
            else
                buffer[index] |= bit;

            // Build the block protection write command
            buffer[0] = CMD_WRITE_BLOCK_PROTECT;

            // Perform the block protection register write operation
            success = r4aSpiTransfer(&spiFlash->_flashChip,
                                     buffer,
                                     nullptr,
                                     length,
                                     display);
            if ((success == false) && display)
                // Display the error
                display->printf("ERROR: Failed to write the block protection register!\r\n");
        } while (0);

        // Disable writes with the WP# pin
        r4aSpiFlashChipWriteEnable(spiFlash, false);
    } while (0);

    // Determine if the write block protection was complete
    return success;
}

//*********************************************************************
// Enable/disable block read access
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   enable: Set true to enable writes and false to prevent writes
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write access was successfully updated and false upon error
bool r4aSpiFlashBlockReadProtectionAll(const R4A_SPI_FLASH * spiFlash,
                                       bool enable,
                                       Print * display)
{
    int bit;
    int bitNumber;
    int entry;
    int index;
    int length = CMD_WRITE_BLOCK_PROTECT_LENGTH + spiFlash->_blockProtectBytes;
    uint8_t buffer[length];
    bool success;

    // Perform the necessary write operations
    success = true;
    do
    {
        // Get the block protect data
        success = r4aSpiFlashBlockProtectionStatus(spiFlash,
                                                   &buffer[CMD_WRITE_BLOCK_PROTECT_LENGTH],
                                                   spiFlash->_blockProtectBytes,
                                                   nullptr);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: Failed to read the block protection register!\r\n");
            break;
        }

        // Walk the list of protection bits
        for (entry = 0;
             spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
             entry++)
        {
            bitNumber = spiFlash->_blockProtect[entry]._readProtectBit;
            if (bitNumber < 0)
                continue;
            index = bitNumber >> 3;
            bit = 1 << (bitNumber & 7);
            index += CMD_WRITE_BLOCK_PROTECT_LENGTH;

            // Update the bit
            if (enable)
                buffer[index] &= ~bit;
            else
                buffer[index] |= bit;
        }

        // Enable writes with the WP# pin
        r4aSpiFlashChipWriteEnable(spiFlash, true);

        // Issue the write enable command
        success = r4aSpiFlashWriteEnable(spiFlash, display);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: SPI Flash write enable command failure!\r\n");
            break;
        }

        // Build the write command
        buffer[0] = CMD_WRITE_BLOCK_PROTECT;

        // Perform the block protection register write operation
        success = r4aSpiTransfer(&spiFlash->_flashChip,
                                 buffer,
                                 nullptr,
                                 length,
                                 display);
        if ((success == false) && display)
            // Display the error
            display->printf("ERROR: Failed to write the block protection register!\r\n");
    } while (0);

    // Disable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, false);

    // Determine if writing all block protections was complete
    return success;
}

//*********************************************************************
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
                                     Print * display)
{
    union
    {
        uint8_t u8[4];
        uint32_t u32;
    } address;
    int bit;
    int bitNumber;
    int length = CMD_WRITE_BLOCK_PROTECT_LENGTH + spiFlash->_blockProtectBytes;
    uint8_t buffer[length];
    int entry;
    int index;
    bool success;

    // Perform the necessary write operations
    address.u32 = flashAddress;
    success = true;
    do
    {
        // Get the block protect data
        success = r4aSpiFlashBlockProtectionStatus(spiFlash,
                                                   &buffer[CMD_WRITE_BLOCK_PROTECT_LENGTH],
                                                   spiFlash->_blockProtectBytes,
                                                   nullptr);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: Failed to read the block protection register!\r\n");
            break;
        }

        // Walk the list of protection bits
        for (entry = 0;
             spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
             entry++)
        {
            // Locate the bit to change
            if ((spiFlash->_blockProtect[entry]._flashAddress <= flashAddress)
                && (spiFlash->_blockProtect[entry + 1]._flashAddress > flashAddress))
            {
                bitNumber = spiFlash->_blockProtect[entry]._writeProtectBit;
                index = bitNumber >> 3;
                bit = 1 << (bitNumber & 7);
                index += CMD_WRITE_BLOCK_PROTECT_LENGTH;
                break;
            }
        }
        if (spiFlash->_blockProtect[entry]._flashAddress >= spiFlash->_flashBytes)
        {
            if (display)
                display->printf("ERROR: Bad flash address, needs to be in the range (0 - 0x%08x)\r\n",
                                spiFlash->_flashBytes);
            success = false;
            break;
        }

        // Skip the write if already in correct state
        if ((buffer[index] & bit) == (enable ? 0 : 1))
            break;

        do
        {
            // Enable writes with the WP# pin
            r4aSpiFlashChipWriteEnable(spiFlash, true);

            // Issue the write enable command
            success = r4aSpiFlashWriteEnable(spiFlash, display);
            if (success == false)
            {
                // Display the error
                if (display)
                    display->printf("ERROR: SPI Flash write enable command failure!\r\n");
                break;
            }

            // Update the bit
            if (enable)
                buffer[index] &= ~bit;
            else
                buffer[index] |= bit;

            // Build the write command
            buffer[0] = CMD_WRITE_BLOCK_PROTECT;

            // Perform the block protection register write operation
            success = r4aSpiTransfer(&spiFlash->_flashChip,
                                     buffer,
                                     nullptr,
                                     length,
                                     display);
            if ((success == false) && display)
                // Display the error
                display->printf("ERROR: Failed to write the block protection register!\r\n");
        } while (0);

        // Disable writes with the WP# pin
        r4aSpiFlashChipWriteEnable(spiFlash, false);
    } while (0);

    // Determine if the write block protection was complete
    return success;
}

//*********************************************************************
// Enable/disable block write access
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   enable: Set true to enable writes and false to prevent writes
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write access was successfully updated and false upon error
bool r4aSpiFlashBlockWriteProtectionAll(const R4A_SPI_FLASH * spiFlash,
                                        bool enable,
                                        Print * display)
{
    int bit;
    int bitNumber;
    int entry;
    int index;
    int length = CMD_WRITE_BLOCK_PROTECT_LENGTH + spiFlash->_blockProtectBytes;
    uint8_t buffer[length];
    bool success;

    // Perform the necessary write operations
    success = true;
    do
    {
        // Get the block protect data
        success = r4aSpiFlashBlockProtectionStatus(spiFlash,
                                                   &buffer[CMD_WRITE_BLOCK_PROTECT_LENGTH],
                                                   spiFlash->_blockProtectBytes,
                                                   nullptr);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: Failed to read the block protection register!\r\n");
            break;
        }

        // Walk the list of protection bits
        for (entry = 0;
             spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
             entry++)
        {
            bitNumber = spiFlash->_blockProtect[entry]._writeProtectBit;
            if (bitNumber < 0)
                continue;
            index = bitNumber >> 3;
            bit = 1 << (bitNumber & 7);
            index += CMD_WRITE_BLOCK_PROTECT_LENGTH;

            // Update the bit
            if (enable)
                buffer[index] &= ~bit;
            else
                buffer[index] |= bit;
        }

        // Enable writes with the WP# pin
        r4aSpiFlashChipWriteEnable(spiFlash, true);

        // Issue the write enable command
        success = r4aSpiFlashWriteEnable(spiFlash, display);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: SPI Flash write enable command failure!\r\n");
            break;
        }

        // Build the write command
        buffer[0] = CMD_WRITE_BLOCK_PROTECT;

        // Perform the block protection register write operation
        success = r4aSpiTransfer(&spiFlash->_flashChip,
                                 buffer,
                                 nullptr,
                                 length,
                                 display);
        if ((success == false) && display)
            // Display the error
            display->printf("ERROR: Failed to write the block protection register!\r\n");
    } while (0);

    // Disable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, false);

    // Determine if writing all block protections was complete
    return success;
}

//*********************************************************************
// Display the block protection
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   dataBuffer: Buffer to receive the block protection
//   display: Address of Print object for output, may be nullptr
void r4aSpiFlashDisplayBlockProtection(const R4A_SPI_FLASH * spiFlash,
                                       uint8_t * dataBuffer,
                                       Print * display)
{
    int bit;
    int entry;
    const uint32_t blockProtectionBits = CMD_READ_BLOCK_PROTECT_DATA << 3;
    int index;
    uint32_t offset;

    // Determine if the entire flash device is write enabled
    for (entry = 0;
         spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
         entry++)
    {
        bit = spiFlash->_blockProtect[entry]._writeProtectBit;
        if (bit < 0)
            continue;
        index = bit >> 3;
        bit = 1 << (bit & 7);
        if (dataBuffer[index] & bit)
            break;
    }
    if (spiFlash->_blockProtect[entry]._flashAddress >= spiFlash->_flashBytes)
        display->printf("Entire SPI Flash is write enabled!\r\n");
    else
    {
        // Display the header
        display->printf("Write Locked Regions\r\n");

        // Display the write protected regions
        for (;
             spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
             entry++)
        {
            // Determine the start of the protected region
            bit = spiFlash->_blockProtect[entry]._writeProtectBit;
            if (bit < 0)
                continue;
            index = bit >> 3;
            bit = 1 << (bit & 7);
            if (dataBuffer[index] & bit)
            {
                uint32_t startAddr;
                uint32_t endAddr;

                startAddr = spiFlash->_blockProtect[entry]._flashAddress;

                // Determine the end of the protected region
                for (entry++;
                     spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
                     entry++)
                {
                    bit = spiFlash->_blockProtect[entry]._writeProtectBit;
                    if (bit < 0)
                        break;
                    index = bit >> 3;
                    bit = 1 << (bit & 7);
                    if ((dataBuffer[index] & bit) == 0)
                        break;
                }
                endAddr = spiFlash->_blockProtect[entry]._flashAddress - 1;
                display->printf("    0x%08x - 0x%08x\r\n", startAddr, endAddr);
            }
        }
    }

    // Determine if the top and bottom portions are read enabled
    for (entry = 0;
         spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
         entry++)
    {
        bit = spiFlash->_blockProtect[entry]._readProtectBit;
        if (bit < 0)
            continue;
        index = bit >> 3;
        bit = 1 << (bit & 7);
        if (dataBuffer[index] & bit)
            break;
    }
    if (spiFlash->_blockProtect[entry]._flashAddress == spiFlash->_flashBytes)
        display->printf("SPI Flash top and bottom are read enabled!\r\n");
    else
    {
        // Display the header
        display->printf("Read Locked Regions\r\n");

        // Display the read protected regions
        for (;
             spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
             entry++)
        {
            // Determine the region is read enabled
            bit = spiFlash->_blockProtect[entry]._readProtectBit;
            if (bit < 0)
                continue;
            index = bit >> 3;
            bit = 1 << (bit & 7);
            if (dataBuffer[index] & bit)
            {
                uint32_t startAddr;
                uint32_t endAddr;

                startAddr = spiFlash->_blockProtect[entry]._flashAddress;

                // Determine the end of the protected region
                for (entry++;
                     spiFlash->_blockProtect[entry]._flashAddress < spiFlash->_flashBytes;
                     entry++)
                {
                    bit = spiFlash->_blockProtect[entry]._readProtectBit;
                    if (bit < 0)
                        break;
                    index = bit >> 3;
                    bit = 1 << (bit & 7);
                    if ((dataBuffer[index] & bit) == 0)
                        break;
                }
                endAddr = spiFlash->_blockProtect[entry]._flashAddress - 1;
                display->printf("    0x%08x - 0x%08x\r\n", startAddr, endAddr);
            }
        }
    }
}

//*********************************************************************
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
                           Print * display)
{
    union
    {
        uint8_t u8[4];
        uint32_t u32;
    } address;
    uint8_t buffer[CMD_ERASE_BLOCK_LENGTH];
    bool success;

    // Enable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, true);

    // Issue the write enable command
    success = r4aSpiFlashWriteEnable(spiFlash, display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash write enable command failure!\r\n");
    }
    else
    {
        // Build the erase command
        address.u32 = flashAddress;
        buffer[0] = CMD_ERASE_BLOCK;
        buffer[1] = address.u8[2];
        buffer[2] = address.u8[1];
        buffer[3] = address.u8[0];

        // Perform the erase block operation
        success = r4aSpiTransfer(&spiFlash->_flashChip,
                                 buffer,
                                 nullptr,
                                 CMD_ERASE_BLOCK_LENGTH,
                                 display);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: SPI Flash erase block failure!\r\n");
        }
        else
        {
            // Wait for the write operation to complete
            do
            {
                success = r4aSpiFlashReadStatusRegister(spiFlash, status, display);
            } while (success && (*status & spiFlash->_stsWriteInProgress));
            if (success)
            {
                *status &= spiFlash->_stsEraseErrors;
                if (*status)
                {
                    // Display the error
                    if (display)
                        display->printf("ERROR: SPI Flash erase block failure!\r\n");
                }
            }
        }
    }

    // Disable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, false);

    // Determine if the erase block was complete
    return success;
}

//*********************************************************************
// Erase the entire SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   status: Address of buffer to receive the final status
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the erase was successful and false upon error
bool r4aSpiFlashEraseChip(const R4A_SPI_FLASH * spiFlash,
                          uint8_t * status,
                          Print * display)
{
    uint8_t buffer[CMD_CHIP_ERASE_LENGTH];
    bool success;

    // Enable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, true);

    // Issue the write enable command
    success = r4aSpiFlashWriteEnable(spiFlash, display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash write enable command failure!\r\n");
    }
    else
    {
        // Build the erase command
        buffer[0] = CMD_CHIP_ERASE;

        // Perform the erase sector operation
        success = r4aSpiTransfer(&spiFlash->_flashChip,
                                 buffer,
                                 nullptr,
                                 CMD_CHIP_ERASE_LENGTH,
                                 display);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: SPI Flash chip erase command failure!\r\n");
            success = false;
        }
        else
        {
            // Wait for the write operation to complete
            do
            {
                success = r4aSpiFlashReadStatusRegister(spiFlash, status, display);
            } while (success && (*status & spiFlash->_stsWriteInProgress));
            if (success)
            {
                *status &= spiFlash->_stsEraseErrors;
                if (*status)
                {
                    // Display the error
                    if (display)
                        display->printf("ERROR: SPI Flash erase failure!\r\n");
                }
            }
        }
    }

    // Disable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, false);

    // Determine if the chip erase was complete
    return success;
}

//*********************************************************************
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
                            Print * display)
{
    union
    {
        uint8_t u8[4];
        uint32_t u32;
    } address;
    uint8_t buffer[CMD_ERASE_SECTOR_LENGTH];
    bool success;

    // Enable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, true);

    // Issue the write enable command
    success = r4aSpiFlashWriteEnable(spiFlash, display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash write enable command failure!\r\n");
    }
    else
    {
        // Build the erase command
        address.u32 = flashAddress;
        buffer[0] = CMD_ERASE_SECTOR;
        buffer[1] = address.u8[2];
        buffer[2] = address.u8[1];
        buffer[3] = address.u8[0];

        // Perform the erase sector operation
        success = r4aSpiTransfer(&spiFlash->_flashChip,
                                 buffer,
                                 nullptr,
                                 CMD_ERASE_SECTOR_LENGTH,
                                 display);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: SPI Flash erase sector failure!\r\n");
        }
        else
        {
            // Wait for the write operation to complete
            do
            {
                success = r4aSpiFlashReadStatusRegister(spiFlash, status, display);
            } while (success && (*status & spiFlash->_stsWriteInProgress));
            if (success)
            {
                *status &= spiFlash->_stsEraseErrors;
                if (*status)
                {
                    // Display the error
                    if (display)
                        display->printf("ERROR: SPI Flash erase sector failure!\r\n");
                }
            }
        }
    }

    // Disable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, false);

    // Determine if the erase sector was complete
    return success;
}

//*********************************************************************
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
                     Print * display)
{
    union
    {
        uint8_t u8[4];
        uint32_t u32;
    } address;
    uint8_t * buffer;
    bool success;

    // Allocate a temporary buffer
    buffer = (uint8_t *)r4aMalloc(length + CMD_READ_LENGTH, "SPI Flash read buffer");
    if (buffer == nullptr)
    {
        if (display)
            display->printf("SPI Flash: Failed to allocate the buffer\r\n");
        return false;
    }

    // Perform the necessary read operations
    address.u32 = flashAddress;

    // Build the read command
    memset(buffer, 0, length + CMD_READ_LENGTH);
    buffer[0] = CMD_READ;
    buffer[1] = address.u8[2];
    buffer[2] = address.u8[1];
    buffer[3] = address.u8[0];

    // Perform the read operation
    success = r4aSpiTransfer(&spiFlash->_flashChip,
                             buffer,
                             buffer,
                             CMD_READ_LENGTH + length,
                             display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash read failure!\r\n");
    }
    else
        // Move the read data into the buffer
        memcpy(dataBuffer, &buffer[CMD_READ_LENGTH], length);

    // Free the temporary buffer
    r4aFree(buffer, "SPI Flash read buffer");

    // Determine if the read was complete
    return success;
}

//*********************************************************************
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
                                        Print * display)
{
    union
    {
        uint8_t u8[4];
        uint32_t u32;
    } address;
    uint8_t * buffer;
    bool success;

    // Allocate a temporary buffer
    buffer = (uint8_t *)r4aMalloc(length + CMD_READ_DISCOVERY_LENGTH,
                                  "SPI Flash read discovery buffer");
    if (buffer == nullptr)
    {
        if (display)
            display->printf("SPI Flash: Failed to allocate the buffer\r\n");
        return false;
    }

    // Perform the necessary read discovery operation
    address.u32 = flashAddress;

    // Build the read discovery command
    memset(buffer, 0, length + CMD_READ_DISCOVERY_LENGTH);
    buffer[0] = CMD_READ_DISCOVERY;
    buffer[1] = address.u8[2];
    buffer[2] = address.u8[1];
    buffer[3] = address.u8[0];

    // Perform the read discovery operation
    success = r4aSpiTransfer(&spiFlash->_flashChip,
                             buffer,
                             buffer,
                             CMD_READ_DISCOVERY_LENGTH + length,
                             display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash read discovery failure!\r\n");
    }
    else
        // Move the read discovery data into the buffer
        memcpy(dataBuffer, &buffer[CMD_READ_DISCOVERY_LENGTH], length);

    // Free the temporary buffer
    r4aFree(buffer, "SPI Flash read discovery buffer");

    // Determine if the read discovery was complete
    return success;
}

//*********************************************************************
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
                         Print * display)
{
    uint8_t * buffer;
    bool success;

    // Allocate a temporary buffer
    buffer = (uint8_t *)r4aMalloc(length + CMD_READ_ID_9E_LENGTH,
                                  "SPI Flash read ID buffer");
    if (buffer == nullptr)
    {
        if (display)
            display->printf("SPI Flash: Failed to allocate the DMA buffer\r\n");
        return false;
    }

    // Build the read ID command
    memset(buffer, 0, length + CMD_READ_ID_9E_LENGTH);
    buffer[0] = CMD_READ_ID_9E;

    // Perform the read ID operation
    success = r4aSpiTransfer(&spiFlash->_flashChip,
                             buffer,
                             buffer,
                             CMD_READ_ID_9E_LENGTH + length,
                             display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash read ID failure!\r\n");
    }
    else
        // Move the ID data into the buffer
        memcpy(dataBuffer, &buffer[CMD_READ_ID_9E_LENGTH], length);

    // Free the temporary buffer
    r4aFree(buffer, "SPI Flash read ID buffer");

    // Determine if the read ID was complete
    return success;
}

//*********************************************************************
// Read the ID from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   dataBuffer: Buffer to receive the SPI flash data of R4A_SPI_FLASH_9F_ID_BYTES
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the read ID was successful and false upon error
bool r4aSpiFlashReadId9f(const R4A_SPI_FLASH * spiFlash,
                         uint8_t * dataBuffer,
                         Print * display)
{
    uint8_t buffer[CMD_READ_ID_9F_LENGTH + R4A_SPI_FLASH_9F_ID_BYTES];
    bool success;

    // Build the read ID command
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = CMD_READ_ID_9F;

    // Perform the read ID operation
    success = r4aSpiTransfer(&spiFlash->_flashChip,
                             buffer,
                             buffer,
                             sizeof(buffer),
                             display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash read ID failure!\r\n");
    }
    else
        // Move the ID data into the buffer
        memcpy(dataBuffer, &buffer[CMD_READ_ID_9F_LENGTH], R4A_SPI_FLASH_9F_ID_BYTES);

    // Determine if the read ID was complete
    return success;
}

//*********************************************************************
// Read status register from the SPI NOR flash
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   status: Buffer to receive the SPI flash status register
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the read was successful and false upon error
bool r4aSpiFlashReadStatusRegister(const R4A_SPI_FLASH * spiFlash,
                                   uint8_t * status,
                                   Print * display)
{
    uint8_t buffer[2];
    bool success;

    // Build the read status command
    buffer[0] = CMD_READ_STATUS;
    buffer[1] = 0;

    // Perform the read status operation
    success = r4aSpiTransfer(&spiFlash->_flashChip,
                             buffer,
                             buffer,
                             CMD_READ_STATUS_LENGTH + 1,
                             display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash read status failure!\r\n");
    }
    else
        // Move the status register value into the status buffer
        *status = buffer[CMD_READ_STATUS_LENGTH];

    // Determine if the read status was complete
    return success;
}

//*********************************************************************
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
                      Print * display)
{
    union
    {
        uint8_t u8[4];
        uint32_t u32;
    } address;
    uint8_t * buffer;
    size_t maxLength;
    bool success;
    size_t transferLength;

    // Allocate a temporary buffer
    buffer = (uint8_t *)r4aMalloc(256 + CMD_WRITE_LENGTH, "SPI Flash write buffer");
    if (buffer == nullptr)
    {
        if (display)
            display->printf("SPI Flash: Failed to allocate the write buffer\r\n");
        return false;
    }

    // Enable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, true);

    // Perform the necessary write operations
    address.u32 = flashAddress;
    success = true;
    do
    {
        // Determine the transfer length
        transferLength = length;
        maxLength = ((~address.u32) & 0xff) + 1;
        if (transferLength > maxLength)
            transferLength = maxLength;

        // Issue the write enable command
        success = r4aSpiFlashWriteEnable(spiFlash, display);
        if (success == false)
        {
            // Display the error
            if (display)
                display->printf("ERROR: SPI Flash write enable command failure!\r\n");
        }
        else
        {
            // Build the write command
            memset(buffer, 0, transferLength + CMD_WRITE_LENGTH);
            buffer[0] = CMD_WRITE;
            buffer[1] = address.u8[2];
            buffer[2] = address.u8[1];
            buffer[3] = address.u8[0];

            // Move the data into the buffer
            memcpy(&buffer[CMD_WRITE_LENGTH], writeBuffer, transferLength);

            // Perform the write operation
            success = r4aSpiTransfer(&spiFlash->_flashChip,
                                     buffer,
                                     nullptr,
                                     CMD_WRITE_LENGTH + transferLength,
                                     display);
            if (success == false)
            {
                // Display the error
                if (display)
                    display->printf("ERROR: SPI Flash write transfer failure!\r\n");
                success = false;
                break;
            }

            // Wait for the write operation to complete
            do
            {
                delay(1);
                success = r4aSpiFlashReadStatusRegister(spiFlash, status, display);
            } while (success && (*status & spiFlash->_stsWriteInProgress));
            if (!success)
                break;
            *status &= spiFlash->_stsProgramErrors;
            if (*status)
            {
                // Display the error
                if (display)
                    display->printf("ERROR: SPI Flash write data failure!\r\n");
                success = false;
            }

            // Account for this write operation
            writeBuffer += transferLength;
            address.u32 += transferLength;
            length -= transferLength;
        }
    } while (success && (length > 0));

    // Disable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, false);

    // Free the temporary buffer
    r4aFree(buffer, "SPI Flash write buffer");

    // Determine if the write was complete
    return success;
}

//*********************************************************************
// Disable write operations
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write disable was successful and false upon error
bool r4aSpiFlashWriteDisable(const R4A_SPI_FLASH * spiFlash,
                             Print * display)
{
    uint8_t buffer;
    bool success;

    // Build the erase command
    buffer = CMD_WRITE_DISABLE;

    // Perform the erase sector operation
    success = r4aSpiTransfer(&spiFlash->_flashChip,
                             &buffer,
                             nullptr,
                             CMD_WRITE_DISABLE_LENGTH,
                             display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash write disable failure!\r\n");
    }

    // Determine if the write disable was complete
    return success;
}

//*********************************************************************
// Write status operations
// Inputs:
//   spiFlash: Address of an R4A_SPI_FLASH data structure
//   status: New status value
//   display: Address of Print object for output, may be nullptr
// Outputs:
//   Returns true if the write status was successful and false upon error
bool r4aSpiFlashWriteStatus(const R4A_SPI_FLASH * spiFlash,
                            uint8_t status,
                            Print * display)
{
    uint8_t buffer[CMD_WRITE_STATUS_LENGTH];
    bool success;

    // Enable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, true);

    // Build the erase command
    buffer[0] = CMD_WRITE_ENABLE;
    buffer[1] = CMD_WRITE_STATUS;
    buffer[2] = status;

    // Perform the erase sector operation
    success = r4aSpiTransfer(&spiFlash->_flashChip,
                             buffer,
                             nullptr,
                             CMD_WRITE_STATUS_LENGTH,
                             display);
    if (success == false)
    {
        // Display the error
        if (display)
            display->printf("ERROR: SPI Flash write status failure!\r\n");
    }

    // Disable writes with the WP# pin
    r4aSpiFlashChipWriteEnable(spiFlash, false);

    // Determine if the write status was complete
    return success;
}

//*********************************************************************
// Enable/disable read access to block containing the specified address
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionRead(const R4A_MENU_ENTRY * menuEntry,
                                        const char * command,
                                        Print * display)
{
    uint32_t address;
    int enable;
    String line;
    uint8_t status;
    int values;

    do
    {
        // Get the command name
        String line = r4aMenuGetParameters(menuEntry, command);
        line.trim();

        // Get the address
        values = sscanf(line.c_str(), "%x %d", &address, &enable);
        if (values != 2)
        {
            if (display)
                display->printf("Please specify the address in hex and an enable (1 or 0)\r\n");
            break;
        }

        // Set the block protection for the specified address
        if (r4aSpiFlashBlockReadProtection(r4aSpiFlash,
                                           address,
                                           enable,
                                           &status,
                                           nullptr) == false)
            display->printf("Failed to update the SPI flash read protections\r\n");
        else
            display->printf("Successfully updated block read protection\r\n");
    } while (0);
}

//*********************************************************************
// Enable/disable read access to all blocks
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionReadAll(const R4A_MENU_ENTRY * menuEntry,
                                           const char * command,
                                           Print * display)
{
    int enable;

    do
    {
        // Get the enable/disable value
        enable = menuEntry->menuParameter & 1;

        // Set all the write block protections
        if (r4aSpiFlashBlockReadProtectionAll(r4aSpiFlash,
                                              enable,
                                              nullptr) == false)
            display->printf("SPI flash device failed to update all block read protections\r\n");
        else
            display->printf("Successfully updated all block read protections\r\n");
    } while (0);
}

//*********************************************************************
// Read block protection register from the SPI flash device
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionStatus(const R4A_MENU_ENTRY * menuEntry,
                                          const char * command,
                                          Print * display)
{
    uint8_t buffer[CMD_READ_BLOCK_PROTECT_DATA];

    // Read the block protection from the SPI flash device
    if (r4aSpiFlashBlockProtectionStatus(r4aSpiFlash,
                                         buffer,
                                         sizeof(buffer),
                                         nullptr))
    {
        // Display the protection data
        r4aDumpBuffer(0, buffer, sizeof(buffer));

        // Decode the protection data
        r4aSpiFlashDisplayBlockProtection(r4aSpiFlash,
                                          buffer,
                                          display);
    }
    else
        display->printf("Failed to read the block protection\r\n");
}

//*********************************************************************
// Enable/disable write access to block containing the specified address
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionWrite(const R4A_MENU_ENTRY * menuEntry,
                                         const char * command,
                                         Print * display)
{
    uint32_t address;
    int enable;
    String line;
    uint8_t status;
    int values;

    do
    {
        // Get the command name
        String line = r4aMenuGetParameters(menuEntry, command);
        line.trim();

        // Get the address
        values = sscanf(line.c_str(), "%x %d", &address, &enable);
        if (values != 2)
        {
            if (display)
                display->printf("Please specify the address in hex and an enable (1 or 0)\r\n");
            break;
        }

        // Set the block protection for the specified address
        if (r4aSpiFlashBlockWriteProtection(r4aSpiFlash,
                                            address,
                                            enable,
                                            &status,
                                            nullptr) == false)
            display->printf("Failed to update the SPI flash write protections\r\n");
        else
            display->printf("Successfully updated block write protection\r\n");
    } while (0);
}

//*********************************************************************
// Enable/disable write access to all blocks
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuBlockProtectionWriteAll(const R4A_MENU_ENTRY * menuEntry,
                                            const char * command,
                                            Print * display)
{
    int enable;

    do
    {
        // Get the enable/disable value
        enable = menuEntry->menuParameter & 1;

        // Set all the write block protections
        if (r4aSpiFlashBlockWriteProtectionAll(r4aSpiFlash,
                                               enable,
                                               nullptr) == false)
            display->printf("SPI flash device failed to update all block write protections\r\n");
        else
            display->printf("Successfully updated all block write protections\r\n");
    } while (0);
}

//*********************************************************************
// Erase a 4K block of the SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuErase4K(const R4A_MENU_ENTRY * menuEntry,
                            const char * command,
                            Print * display)
{
    uint32_t address;
    String line;
    uint8_t status;
    int values;

    do
    {
        // Get the command name
        String line = r4aMenuGetParameters(menuEntry, command);
        line.trim();

        // Get the address
        values = sscanf(line.c_str(), "%x", &address);
        if (values != 1)
        {
            if (display)
                display->printf("Please specify the address in hex\r\n");
            break;
        }

        // Erase a 4K block of the SPI flash
        if (r4aSpiFlashEraseSector(r4aSpiFlash, address, &status, nullptr) == false)
            display->printf("SPI flash device failed to erase 4K\r\n");
        else
        {
            // Determine if the erase was successful
            if ((status & r4aSpiFlash->_stsEraseErrors) == 0)
                display->printf("Successfully erased 4K bytes at 0x%08x\r\n",
                                address & 0xfffff000);
            else
                // No, display the error status
                r4aSpiFlash->_displayStatus(status, display);
        }
    } while (0);
}

//*********************************************************************
// Erase a 65K block of the SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuErase65K(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    uint32_t address;
    String line;
    uint8_t status;
    int values;

    do
    {
        // Get the command name
        String line = r4aMenuGetParameters(menuEntry, command);
        line.trim();

        // Get the address
        values = sscanf(line.c_str(), "%x", &address);
        if (values != 1)
        {
            if (display)
                display->printf("Please specify the address in hex\r\n");
            break;
        }

        // Erase an 8K, 32K or 65K block of the SPI flash
        if (r4aSpiFlashEraseBlock(r4aSpiFlash, address, &status, nullptr) == false)
            display->printf("SPI flash device failed to erase 65K\r\n");
        else
        {
            // Determine if the erase was successful
            if ((status & r4aSpiFlash->_stsEraseErrors) == 0)
                display->printf("Successfully erased 65K bytes at 0x%08x\r\n",
                                address & 0xffff0000);
            else
                // Display the final status
                r4aSpiFlash->_displayStatus(status, display);
        }
    } while (0);
}

//*********************************************************************
// Erase the entire SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuEraseChip(const R4A_MENU_ENTRY * menuEntry,
                              const char * command,
                              Print * display)
{
    uint8_t status;

    // Erase the SPI flash
    if (r4aSpiFlashEraseChip(r4aSpiFlash, &status, nullptr) == false)
        display->printf("SPI flash device failed to erase 65K\r\n");
    else
    {
        // Determine if the erase was successful
        if ((status & r4aSpiFlash->_stsEraseErrors) == 0)
            display->printf("Successfully erased the chip\r\n");
        else
            // Display the final status
            r4aSpiFlash->_displayStatus(status, display);
    }
}

//*********************************************************************
// Read data from the SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadData(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    uint32_t address;
    uint8_t * buffer;
    uint32_t length;
    String line;
    int values;

    do
    {
        buffer = nullptr;

        // Get the command name
        String line = r4aMenuGetParameters(menuEntry, command);
        line.trim();

        // Get the address and length parameters
        values = sscanf(line.c_str(), "%x %x", &address, &length);
        if (values != 2)
        {
            if (display)
                display->printf("Please specify the address and length in hex\r\n");
            break;
        }

        // Allocate the buffer to receive the data
        buffer = (uint8_t *)r4aMalloc(length, "Menu SPI read buffer");
        if (buffer == nullptr)
        {
            display->printf("Failed to allocate the read buffer!\r\n");
            break;
        }

        // Read the data from the SPI flash
        if (r4aSpiFlashRead(r4aSpiFlash, address, buffer, length, nullptr))
            r4aDumpBuffer(address, buffer, length);
        else
            display->printf("Failed to read data from the SPI flash device\r\n");
    } while (0);

    // Free the buffer
    if (buffer)
        r4aFree(buffer, "Menu SPI read buffer");
}

//*********************************************************************
// Read ID from the SPI flash device
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadId9e(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    uint8_t buffer[20];
    uint8_t ff[20];

    // Read the ID from the SPI flash device
    if (r4aSpiFlashReadId9e(r4aSpiFlash, buffer, sizeof(buffer), nullptr))
    {
        // Check for unknown command
        memset(ff, 0xff, sizeof(ff));
        if (memcmp(buffer, ff, sizeof(buffer)) == 0)
            display->printf("SPI Flash does not support command 0x9e!\r\n");
        else
        {
            // Display the raw data
            r4aDumpBuffer(0, buffer, sizeof(buffer));

            // Decode the ID data
            display->printf("0x%02x: Manufacture\r\n", buffer[0]);
            display->printf("0x%02x: Memory Type\r\n", buffer[1]);
            display->printf("0x%02x: Memory Capacity - %d MB\r\n",
                            buffer[2], 1 << (buffer[2] - 20));
        }
    }
    else
        display->printf("Failed to read the device ID\r\n");
}

//*********************************************************************
// Read ID from the SPI flash device
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadId9f(const R4A_MENU_ENTRY * menuEntry,
                             const char * command,
                             Print * display)
{
    uint8_t buffer[3];

    // Read the ID from the SPI flash device
    if (r4aSpiFlashReadId9f(r4aSpiFlash, buffer, nullptr))
    {
        // Display the raw data
        r4aDumpBuffer(0, buffer, sizeof(buffer));

        // Decode the ID data
        display->printf("0x%02x: Manufacture\r\n", buffer[0]);
        display->printf("0x%02x: Memory Type\r\n", buffer[1]);
        display->printf("0x%02x: Memory Capacity\r\n", (buffer[2]));
    }
    else
        display->printf("Failed to read the device ID\r\n");
}

//*********************************************************************
// Read the discovery parameters from the SPI flash
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadParameters(const R4A_MENU_ENTRY * menuEntry,
                                   const char * command,
                                   Print * display)
{
    uint32_t address;
    uint8_t * buffer;
    uint32_t length;
    String line;
    int values;

    do
    {
        buffer = nullptr;

        // Get the command name
        String line = r4aMenuGetParameters(menuEntry, command);
        line.trim();

        // Get the address and length
        values = sscanf(line.c_str(), "%x %x", &address, &length);
        if (values != 2)
        {
            if (display)
                display->printf("Please specify the address and length in hex\r\n");
            break;
        }

        // Allocate the buffer to receive the data
        buffer = (uint8_t *)r4aMalloc(length, "Menu SPI read discovery parameters buffer");
        if (buffer == nullptr)
        {
            display->printf("Failed to allocate the read buffer!\r\n");
            break;
        }

        // Read the data from the SPI flash
        if (r4aSpiFlashReadDiscoveryParameters(r4aSpiFlash,
                                               address,
                                               buffer,
                                               length,
                                               nullptr))
            r4aDumpBuffer(address, buffer, length);
        else
            display->printf("Failed to read discovery parameters from the SPI flash device\r\n");
    } while (0);

    // Free the buffer
    if (buffer)
        r4aFree(buffer, "Menu SPI read discovery parameters buffer");
}

//*********************************************************************
// Read and display the status register
// Inputs:
//   menuEntry: Address of an R4A_MENU_ENTRY data structure
//   command: Address of a zero terminated string containing the command
//   display: Address of the serial output device
void r4aSpiFlashMenuReadStatusRegister(const R4A_MENU_ENTRY * menuEntry,
                                       const char * command,
                                       Print * display)
{
    uint8_t status;

    // Read the status from the SPI flash device
    if (r4aSpiFlashReadStatusRegister(r4aSpiFlash, &status, nullptr))
    {
        // Display the status
        r4aSpiFlash->_displayStatus(status, display);
    }
    else
        display->printf("Failed to read the status register!\r\n");
}

//*********************************************************************
// Enable or disable SPI NOR Flash writes using the WP# pin
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   command: Zero terminated command string
//   display: Device used for output
void r4aSpiFlashMenuWriteEnable(const R4A_MENU_ENTRY * menuEntry,
                                const char * command,
                                Print * display)
{
    r4aSpiFlashChipWriteEnable(r4aSpiFlash, menuEntry->menuParameter & 1);
}
