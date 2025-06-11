/*
 * crc.cpp
 *
 *  Created on: Jun 6, 2025
 *      Author: Fki
 */

#include "crc.hpp"

uint16_t crc;

bool checkCRC(uint8_t buf[], int16_t len) {
  crc = 0xFFFF;
  for (uint8_t i = 0; i < len - 2; i++) {
    calculateCRC(buf[i]);
  }
  if (highByte(crc) == buf[len - 1] && lowByte(crc) == buf[len - 2]) {
    return true;
  } else {
    return false;
  }
}

void calculateCRC(uint8_t b) {
  crc ^= (uint16_t)b;              // XOR byte into least sig. byte of crc
  for (uint8_t i = 8; i != 0; i--) {  // Loop over each bit
    if ((crc & 0x0001) != 0) {     // If the LSB is set
      crc >>= 1;                   // Shift right and XOR 0xA001
      crc ^= 0xA001;
    } else        // Else LSB is not set
      crc >>= 1;  // Just shift right
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
}

