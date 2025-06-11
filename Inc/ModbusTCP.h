/*
 * ModbusTCP.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Fki
 */

#ifndef MODBUSTCP_H_
#define MODBUSTCP_H_

#include <cstdint>

#define PRIORITY_REQUEST 0x80   //B1000 0000  // Request to slave which is not "nonresponding"
#define SCAN_REQUEST 0x40       //B0100 0000      // Request triggered by slave scanner
#define UDP_REQUEST 0x20        //B0010 0000       // UDP request
#define TCP_REQUEST 0x08        //B0000 1000       // TCP request
#define TCP_REQUEST_MASK 0x07   //B0000 0111  // Stores TCP client number

extern uint8_t socketInQueue[MAX_SOCK_NUM];

void startEthernet();
void scanRequest();
void recvUdp();
uint8_t checkRequest(uint8_t inBuffer[], uint16_t msgLength, const uint32_t remoteIP, const uint16_t remotePort, uint8_t requestType);
void setSlaveStatus(const uint8_t slave, uint8_t status, const bool value, const bool isScan);
bool getSlaveStatus(const uint8_t slave, const uint8_t status);
void manageSockets();
void recvTcp(EthernetClient &client);
void disconSocket(uint8_t s);
void sendResponse(const uint8_t MBAP[], const uint8_t PDU[], const uint16_t pduLength);
void deleteRequest();

#endif /* MODBUSTCP_H_ */
