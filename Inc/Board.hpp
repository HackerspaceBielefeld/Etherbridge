/*
 * Board.hpp
 *
 *  Created on: May 16, 2025
 *      Author: Fki
 */

#ifndef INC_BOARD_HPP_
#define INC_BOARD_HPP_

#include "SPI_Master.h"
#include "SPI_Channel.h"
#include "RS485.hpp"
#include "Eeprom93LC86.hpp"

extern uint32_t SYS_uid[3];

extern SPI_Master wzSPI;
extern SPI_Channel wzChannel;
extern FastIo wzRstPin;

extern SPI_Master eepSpi;
extern SPI_Channel eepIf;
extern Eeprom93LC86 eeprom;

extern RS485<256> modbus;

void BRD_init(void);


#endif /* INC_BOARD_HPP_ */
