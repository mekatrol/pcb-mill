#include <stdint.h>

#include "../hal/uart.h"

// RX buffer
#define RX_BUF_SIZE 64
extern volatile uint8_t rx_buf[RX_BUF_SIZE];
extern volatile uint8_t rx_index;

uint8_t tmc_crc8(uint8_t *data, uint8_t length)
{
    uint8_t crc = 0;
    uint8_t currentByte;

    for (uint8_t i = 0; i < length; i++)
    {                          // Execute for all bytes of a message
        currentByte = data[i]; // Retrieve a byte to be sent from Array
        for (uint8_t j = 0; j < 8; j++)
        {
            if ((crc >> 7) ^ (currentByte & 0x01)) // update CRC based result of XOR operation
            {
                crc = (crc << 1) ^ 0x07;
            }
            else
            {
                crc = (crc << 1);
            }
            currentByte = currentByte >> 1;
        } // for CRC bit
    } // for message byte

    return crc;
}

void tmc2209_read_gconf(uint8_t slave)
{
    uint8_t packet[4];
    packet[0] = 0x05;                // Sync nibble
    packet[1] = slave;               // Slave addr
    packet[2] = 0x00;                // Register address (GCONF) + read b7=1
    packet[3] = tmc_crc8(packet, 3); // CRC

    for (int i = 0; i < 4; i++)
    {
        uart4_send(packet[i]);
    }
}

int tmc2209_parse_reply(uint8_t sent_count, uint8_t *data_out)
{
    // Note, as the E3 Mini used onw wire on RX4 then the transmitted bytes will be echoed
    // via the one wire resitor, so we need to ignore first 'sent_count' bytes of received data (becayse we transmitted it)

    if (rx_index < 8 + sent_count)
        return -1; // Not enough data

    // Check sync
    if (rx_buf[0 + sent_count] != 0x05)
        return -2;

    // Check addr is 0xFF which is reserved for responses to master
    if (rx_buf[1 + sent_count] != 0xFF)
        return -3;

    // Check register
    if (rx_buf[2 + sent_count] != 0x00)
        return -4;

    // Check CRC
    uint8_t calc_crc = tmc_crc8((uint8_t *)(&rx_buf[sent_count]), 7);
    if (rx_buf[7 + sent_count] != calc_crc)
        return -5;

    // Copy 4-byte GCONF register value (LSB first)
    for (int i = 0; i < 4; i++)
        data_out[i] = rx_buf[3 + i + sent_count];

    return 0;
}