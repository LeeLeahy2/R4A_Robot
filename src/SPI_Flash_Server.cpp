/**********************************************************************
  SPI_Flash_Server.cpp

  Robots-For-All (R4A)
  SPI NOR flash server support
**********************************************************************/

#include "R4A_Robot.h"

//****************************************
// Globals
//****************************************

NetworkClient r4aSpiFlashClient;
uint8_t * r4aSpiFlashClientBuffer;
R4A_SPI_FLASH_COMMAND r4aSpiFlashClientCmd;
size_t r4aSpiFlashClientBytesProcessed;
size_t r4aSpiFlashClientBytesValid;
NetworkServer * r4aSpiFlashServer;

//*********************************************************************
// Close the client
void r4aSpiFlashClientClose()
{
    if (r4aSpiFlashClient)
        r4aSpiFlashClient.stop();
}

//*********************************************************************
// Determine if the telnet server has any clients
// Outputs:
//   Returns true if one or more clients are connected.
bool r4aSpiFlashClientConnected()
{
    if (r4aSpiFlashClient)
        return r4aSpiFlashClient.connected();
    return false;
}

//*********************************************************************
// Create a new client
void r4aSpiFlashClientNew()
{
    // Get the remote client connection
    r4aSpiFlashClient = r4aSpiFlashServer->accept();
}

//*********************************************************************
// Done processing command
void r4aSpiFlashClientProcessDone()
{
    if (r4aSpiFlashClientBuffer)
    {
        r4aFree(r4aSpiFlashClientBuffer, "SPI Flash client buffer");
        r4aSpiFlashClientBuffer = nullptr;
    }
    r4aSpiFlashClientBytesProcessed = 0;
    r4aSpiFlashClientBytesValid = 0;
    memset(&r4aSpiFlashClientCmd, 0, sizeof(r4aSpiFlashClientCmd));
}

//*********************************************************************
// Get the command from the remote application
bool r4aSpiFlashClientProcessInput()
{
    size_t bytesTransferred;
    R4A_SPI_FLASH_COMMAND cmd;
    bool enable;
    uint8_t status;
    size_t transferLength;

    if (r4aSpiFlashClient.connected())
    {
        switch (r4aSpiFlashClientCmd._command)
        {
        default:
            Serial.printf("Unknown command: 0x%02x, address: 0x%08x, bytes: 0x%04x\r\n",
                r4aSpiFlashClientCmd._command,
                r4aSpiFlashClientCmd._flashAddress,
                r4aSpiFlashClientCmd._lengthInBytes);
            break;

        // Get the command from the host
        case CMD_READ_COMMAND:
            if (r4aSpiFlashClient.available() >= sizeof(r4aSpiFlashClientCmd))
            {
                r4aSpiFlashClient.read((uint8_t *)&r4aSpiFlashClientCmd, sizeof(r4aSpiFlashClientCmd));
                r4aSpiFlashClientBytesProcessed = 0;
                r4aSpiFlashClientBytesValid = 0;

                // Allocate the buffer if necessary
                if (r4aSpiFlashClientBuffer == nullptr)
                {
                    if (r4aSpiFlashClientCmd._lengthInBytes > 0)
                    {
                        r4aSpiFlashClientBuffer = (uint8_t *)r4aMalloc(r4aSpiFlashClientCmd._lengthInBytes,
                                                                       "SPI Flash client buffer");
                        if (r4aSpiFlashClientBuffer == nullptr)
                        {
                            Serial.printf("ERROR: Failed to allocate the SPI flash client buffer!\r\n");
                            break;
                        }
                    }
                }
            }
            return true;

        // Read the data from the flash device and transfer it to the host
        case CMD_READ_DATA:
            if (r4aSpiFlashClientBytesValid == 0)
            {
                // Read the data from the flash if necessary
                if (r4aSpiFlashRead(r4aSpiFlash,
                                    r4aSpiFlashClientCmd._flashAddress,
                                    r4aSpiFlashClientBuffer,
                                    r4aSpiFlashClientCmd._lengthInBytes,
                                    nullptr) == false)
                {
                    Serial.printf("ERROR: Failed to read %d bytes from SPI flash at 0x%08x!\r\n",
                                  r4aSpiFlashClientCmd._lengthInBytes, r4aSpiFlashClientCmd._flashAddress);
                    break;
                }
                r4aSpiFlashClientBytesProcessed = 0;
                r4aSpiFlashClientBytesValid = r4aSpiFlashClientCmd._lengthInBytes;
            }

            // Transfer the data to the host
            bytesTransferred = r4aSpiFlashClient.write(&r4aSpiFlashClientBuffer[r4aSpiFlashClientBytesProcessed],
                                                       r4aSpiFlashClientCmd._lengthInBytes);
            if (bytesTransferred)
            {
                r4aSpiFlashClientBytesProcessed += bytesTransferred;
                r4aSpiFlashClientCmd._lengthInBytes -= bytesTransferred;
                r4aSpiFlashClientCmd._flashAddress += bytesTransferred;
                if (r4aSpiFlashClientCmd._lengthInBytes == 0)
                    r4aSpiFlashClientProcessDone();
            }
            return true;

        case CMD_WRITE_DATA:
            // Get the write data from the host
            if (r4aSpiFlashClientBytesValid != r4aSpiFlashClientCmd._lengthInBytes)
            {
                bytesTransferred = r4aSpiFlashClient.available();
                if (bytesTransferred)
                {
                    // Determine how much data remains
                    if (bytesTransferred > r4aSpiFlashClientCmd._lengthInBytes - r4aSpiFlashClientBytesValid)
                        bytesTransferred = r4aSpiFlashClientCmd._lengthInBytes - r4aSpiFlashClientBytesValid;

                    // Get the data from the host
                    r4aSpiFlashClient.read((uint8_t *)&r4aSpiFlashClientBuffer[r4aSpiFlashClientBytesValid],
                                           bytesTransferred);
                    r4aSpiFlashClientBytesValid += bytesTransferred;
                }
            }
            else
            {
                // Write the data to the SPI flash
                if (r4aSpiFlashWrite(r4aSpiFlash,
                                     r4aSpiFlashClientCmd._flashAddress,
                                     r4aSpiFlashClientBuffer,
                                     r4aSpiFlashClientCmd._lengthInBytes,
                                     &status,
                                     nullptr) == false)
                {
                    Serial.printf("ERROR: Failed to write %d bytes to SPI flash at 0x%08x!\r\n",
                                  r4aSpiFlashClientCmd._lengthInBytes, r4aSpiFlashClientCmd._flashAddress);
                    break;
                }

                // Send the response
                cmd._command = CMD_WRITE_SUCCESS;
                cmd._flashAddress = 0;
                cmd._lengthInBytes = status;
                r4aSpiFlashClient.write((uint8_t *)&cmd, sizeof(cmd));

                // Command complete
                r4aSpiFlashClientProcessDone();
            }
            return true;

        case CMD_ERASE_CHIP:
            // Erase the entire SPI flash chip
            if (r4aSpiFlashEraseChip(r4aSpiFlash, &status, nullptr) == false)
            {
                Serial.printf("ERROR: Failed to erase SPI flash chip!\r\n");
                break;
            }

            // Send the response
            cmd._command = CMD_ERASE_SUCCESS;
            cmd._flashAddress = 0;
            cmd._lengthInBytes = status;
            r4aSpiFlashClient.write((uint8_t *)&cmd, sizeof(cmd));

            // Command complete
            r4aSpiFlashClientProcessDone();
            return true;

        case CMD_BLOCK_WRITE_ENABLE:
            // Get the write enable/disable
            enable = cmd._flashAddress & 1;

            // Update the block write enables
            if (r4aSpiFlashBlockWriteProtectionAll(r4aSpiFlash,
                                                   enable,
                                                   nullptr) == false)
            {
                Serial.printf("ERROR: Failed to update the block write enables!\r\n");
                break;
            }

            // Send the response
            cmd._command = CMD_BLOCK_ENABLE_SUCCESS;
            cmd._flashAddress = 0;
            cmd._lengthInBytes = 0;
            r4aSpiFlashClient.write((uint8_t *)&cmd, sizeof(cmd));

            // Command complete
            r4aSpiFlashClientProcessDone();
            return true;
        }
    }

    // Done with the command
    r4aSpiFlashClientProcessDone();

    // Break the network connection due to the error
    return false;
}

//*********************************************************************
// Initialize the SPI NOR Flash server
// Returns true following successful server initialization and false
// upon failure.
bool r4aSpiFlashServerBegin(IPAddress ipAddress, uint16_t port)
{
    size_t length;

    // Determine if the server was already initialized
    do
    {
        // Allocate the network object
        r4aSpiFlashServer = new NetworkServer(ipAddress, port);
        if (!r4aSpiFlashServer)
        {
            Serial.printf("ERROR: Failed to allocate NetworkServer object!\r\n");
            break;
        }

        // Initialize the network server object
        r4aSpiFlashServer->begin(port);
        r4aSpiFlashServer->setNoDelay(true);
        break;
    } while (0);

    // Notify the caller of the initialization status
    return (r4aSpiFlashServer != nullptr);
}

//*********************************************************************
// Done with the SPI NOR Flash server
void r4aSpiFlashServerEnd()
{
    // Done with the client
    r4aSpiFlashClientProcessDone();
    r4aSpiFlashClientClose();

    // Done with the server
    if (r4aSpiFlashServer)
    {
        delete r4aSpiFlashServer;
        r4aSpiFlashServer = nullptr;
    }
}

//*********************************************************************
// Update the server state
void r4aSpiFlashServerUpdate(bool connected)
{
    // Determine if the network is still working
    if (connected)
    {
        // Check if there are any new clients
        if (r4aSpiFlashServer && (r4aSpiFlashClientConnected() == false))
            r4aSpiFlashClientNew();

        // Check client for data
        if (r4aSpiFlashClient)
        {
            // Process any incoming data, return the connection status
            if (r4aSpiFlashClientProcessInput() == false)

                // Broken connection, free this slot
                r4aSpiFlashClientClose();
        }
    }

    // The network connection is broken
    else if (r4aSpiFlashClient)
        // Disconnected client, free this slot
        r4aSpiFlashClientClose();
}
