/*
 * main.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Fki
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "CircularBuffer.hpp"
#include "ProjectSettings.h"
#include "MicroTimer.hpp"
#include "EthernetServer.h"

typedef struct {
  uint8_t ip[4];
  uint8_t subnet[4];
  uint8_t gateway[4];
  uint8_t dns[4];      // only used if ENABLE_DHCP
  bool enableDhcp;  // only used if ENABLE_DHCP
  uint16_t tcpPort;
  uint16_t udpPort;
  uint16_t webPort;
  bool enableRtuOverTcp;
  uint16_t tcpTimeout;
  uint8_t frameDelay;
  uint16_t serialTimeout;
  uint8_t serialAttempts;
} config_t;

enum status_t : uint8_t {
  SLAVE_OK,              // Slave Responded
  SLAVE_ERROR_0X,        // Slave Responded with Error (Codes 1~8)
  SLAVE_ERROR_0A,        // Gateway Overloaded (Code 10)
  SLAVE_ERROR_0B,        // Slave Failed to Respond (Code 11)
  SLAVE_ERROR_0B_QUEUE,  // Slave Failed to Respond (Code 11) & is in Queue (not shown in web UI)
  ERROR_TIMEOUT,         // Response Timeout
  ERROR_RTU,             // Invalid RTU Response
  ERROR_TCP,             // Invalid TCP/UDP Request
  ERROR_LAST             // Number of status flags in this enum. Must be the last element within this enum!!
};

typedef struct {
    uint8_t tid[2];       // MBAP Transaction ID
    uint8_t msgLen;       // lenght of Modbus message stored in queueData
  IPAddress remIP;   // remote IP for UDP client (UDP response is sent back to remote IP)
  uint16_t remPort;  // remote port for UDP client (UDP response is sent back to remote port)
  uint8_t requestType;  // TCP client who sent the request
  uint8_t atts;         // attempts counter
} header_t;

typedef struct {
  uint32_t eepromWrites;          // Number of EEPROM write cycles (persistent, it is never cleared during factory resets)
  uint8_t major;                  // major version
  uint8_t mac[6];                 // MAC Address (initial value is random generated)
  config_t config;                // configuration values
  uint32_t errorCnt[ERROR_LAST];  // array for storing error counters
#ifdef ENABLE_EXTENDED_WEBUI
  uint32_t rtuCnt[DATA_LAST];  // array for storing RTU data counters
  uint32_t ethCnt[DATA_LAST];  // array for storing ethernet data counters
#endif                         /* ENABLE_EXTENDED_WEBUI */
} data_t;

const config_t DEFAULT_CONFIG = {
  DEFAULT_STATIC_IP,
  DEFAULT_SUBMASK,
  DEFAULT_GATEWAY,
  DEFAULT_DNS,
  DEFAULT_AUTO_IP,
  DEFAULT_TCP_PORT,
  DEFAULT_UDP_PORT,
  DEFAULT_WEB_PORT,
  DEFAULT_RTU_OVER_TCP,
  DEFAULT_TCP_TIMEOUT,
  DEFAULT_FRAME_DELAY,
  DEFAULT_RESPONSE_TIMEPOUT,
  DEFAULT_ATTEMPTS,
};

enum state_t : uint8_t {
  IDLE,
  SENDING,
  DELAY,
  WAITING
};


#ifdef ENABLE_DHCP
extern bool dhcpSuccess;
#endif /* ENABLE_DHCP */

extern data_t data;

extern uint8_t slaveStatus[SLAVE_ERROR_0B_QUEUE + 1][(MAX_SLAVES + 1 + 7) / 8];

extern uint8_t priorityReqInQueue;

extern uint16_t queueDataSize;
extern uint8_t queueHeadersSize;

extern uint8_t scanCounter;
extern bool scanReqInQueue;

extern CircularBuffer<header_t, MAX_QUEUE_REQUESTS> queueHeaders;
extern CircularBuffer<uint8_t, MAX_QUEUE_DATA> queueData;

extern uint8_t maxSockNum;
extern EthernetUDP Udp;
extern EthernetServer modbusServer;
extern EthernetServer webServer;

extern uint8_t responseLen;
extern uint8_t response[MAX_RESPONSE_LEN];

extern MicroTimer recvMicroTimer;
extern MicroTimer sendMicroTimer;

extern uint8_t serialState;

#endif /* MAIN_H_ */
