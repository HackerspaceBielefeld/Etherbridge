/*
 * ModbusRTU.h
 *
 *  Created on: Jun 6, 2025
 *      Author: Fki
 */

#ifndef MODBUSRTU_H_
#define MODBUSRTU_H_

#include <cstdint>

void sendSerial();
void recvSerial();

uint32_t charTimeOut();

#endif /* MODBUSRTU_H_ */
