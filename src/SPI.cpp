/**********************************************************************
  SPI.cpp

  Robots-For-All (R4A)
  SPI support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Transfer the data to the SPI device
bool r4aSpiTransfer(const R4A_SPI_DEVICE * spiDevice,
                    const uint8_t * txBuffer,
                    uint8_t * rxBuffer,
                    size_t length,
                    Print * display)
{
    uint8_t * rxDmaBuffer;
    struct _R4A_SPI_BUS * spiBus;
    bool success;
    spi_transaction_t transaction;
    uint8_t * txDmaBuffer;

    do
    {
        // Get the SPI bus
        spiBus = spiDevice->_spiBus;
        success = false;

        // Describe the SPI transaction
        rxDmaBuffer = nullptr;
        txDmaBuffer = nullptr;
        memset(&transaction, 0, sizeof(transaction));
        transaction.length = length << 3;
        transaction.tx_buffer = txBuffer;
        if (txBuffer)
        {
            // Enable SPI transmit data (MOSI)
            txDmaBuffer = (uint8_t *)r4aDmaMalloc(length, "SPI TX DMA buffer");
            transaction.tx_buffer = txDmaBuffer;
            if (transaction.tx_buffer == nullptr)
            {
                if (display)
                    display->printf("ERROR: Failed to allocate %d byte TX DMA buffer for SPI transaction\r\n", length);
                break;
            }

            // Copy the transmit data into the TX DMA buffer
            memcpy(txDmaBuffer, txBuffer, length);
            if (display)
                r4aDumpBuffer((intptr_t)txDmaBuffer, (uint8_t *)txDmaBuffer, length);
        }

        if (rxBuffer)
        {
            // Enable SPI receive data (MISO)
            rxDmaBuffer = (uint8_t *)r4aDmaMalloc(length, "SPI RX DMA buffer");
            transaction.rx_buffer = rxDmaBuffer;
            if (transaction.rx_buffer == nullptr)
            {
                if (display)
                    display->printf("ERROR: Failed to allocate %d byte RX DMA buffer for SPI transaction\r\n", length);
                break;
            }
        }

        // Perform the SPI transaction
        success = spiBus->_transfer(spiBus, txDmaBuffer, rxDmaBuffer, length, display);
        if (success == false)
            break;

        // Move the SPI receive data into the receive buffer
        if (rxBuffer)
        {
            memcpy((void *)rxBuffer, rxDmaBuffer, length);

            // Display the receive data
            if (display)
                r4aDumpBuffer((uintptr_t)rxBuffer, rxBuffer, length);
        }
    } while (0);

    // Free the DMA buffers
    if (rxDmaBuffer)
        r4aDmaFree(rxDmaBuffer, "SPI RX DMA buffer");
    if (txDmaBuffer)
        r4aDmaFree(txDmaBuffer, "SPI TX DMA buffer");

    // Return the transaction status
    return success;
}
