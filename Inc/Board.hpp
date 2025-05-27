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

extern SPI_Master wzSPI;
extern SPI_Channel wzIf;

extern SPI_Master eepSpi;
extern SPI_Channel eepIf;

void BRD_init(void);


#endif /* INC_BOARD_HPP_ */
