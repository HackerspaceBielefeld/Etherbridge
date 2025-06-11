/*
 * crc.hpp
 *
 *  Created on: Jun 6, 2025
 *      Author: Fki
 */

#ifndef CRC_HPP_
#define CRC_HPP_

#include <cstdint>

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

extern uint16_t crc;

bool checkCRC(uint8_t buf[], int16_t len);
void calculateCRC(uint8_t b);

#endif /* CRC_HPP_ */
