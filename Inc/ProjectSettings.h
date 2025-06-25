/*
 * ProjectSettings.h
 *
 *  Created on: Jun 3, 2025
 *      Author: Fki
 */

#ifndef PROJECTSETTINGS_H_
#define PROJECTSETTINGS_H_

#include <cstdint>

#define ENABLE_DHCP

/****** IP Settings ******/
const bool DEFAULT_AUTO_IP = true;
#define DEFAULT_STATIC_IP \
  { 192, 168, 133, 254 }  // Default Static IP
#define DEFAULT_SUBMASK \
  { 255, 255, 255, 0 }  // Default Submask
#define DEFAULT_GATEWAY \
  { 192, 168, 133, 1 }  // Default Gateway
#define DEFAULT_DNS \
  { 192, 168, 133, 1 }  // Default DNS Server (only used if ENABLE_DHCP)

/****** TCP/UDP Settings ******/
const uint16_t DEFAULT_TCP_PORT = 502;     // Default Modbus TCP Port
const uint16_t DEFAULT_UDP_PORT = 502;     // Default Modbus UDP Port
const uint16_t DEFAULT_WEB_PORT = 80;      // Default WebUI Port
const bool DEFAULT_RTU_OVER_TCP = false;   // Default Modbus Mode (Modbus TCP or Modbus RTU over TCP)
const uint16_t DEFAULT_TCP_TIMEOUT = 600;  // Default Modbus TCP Idle Timeout

/****** RTU Settings ******/
const uint8_t DEFAULT_FRAME_DELAY = 150;            // Default Inter-frame Delay
const uint16_t DEFAULT_RESPONSE_TIMEPOUT = 500;  // Default Response Timeout
const uint8_t DEFAULT_ATTEMPTS = 3;                 // Default Attempts
const uint16_t SCAN_TIMEOUT = 200;       // Timeout (ms) for Modbus scan requests

/****** ADVANCED SETTINGS ******/
const uint8_t MAX_QUEUE_REQUESTS = 10;      // max number of TCP or UDP requests stored in a queue
const uint16_t MAX_QUEUE_DATA = 254;     // total length of TCP or UDP requests stored in a queue (in bytes),
                                         // should be at least MODBUS_SIZE - 2 (CRC is not stored in queue)
const uint16_t MAX_SLAVES = 247;         // max number of Modbus slaves (Modbus supports up to 247 slaves, the rest is for reserved addresses)
const uint16_t MODBUS_SIZE = 256;        // maximum size of a MODBUS RTU frame incl slave address and CRC (determines size of various buffers)

const uint8_t MAX_RESPONSE_LEN = 16;        // Max length (bytes) of the Modbus response shown in WebUI

const uint16_t ETH_RESET_DELAY = 200;            // Delay (ms) during Ethernet start, wait for Ethernet shield to start (reset issue on low quality ethernet shields)
const uint16_t TCP_RETRANSMISSION_TIMEOUT = 50;  // Ethernet controller’s timeout (ms), blocking (see https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.setretransmissiontimeout/)
const uint8_t TCP_RETRANSMISSION_COUNT = 3;         // Number of transmission attempts the Ethernet controller will make before giving up (see https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.setretransmissioncount/)
const uint8_t SCAN_FUNCTION_FIRST = 0x03;   // Function code sent during Modbus RTU Scan request (first attempt)
const uint8_t SCAN_FUNCTION_SECOND = 0x04;  // Function code sent during Modbus RTU Scan request (second attempt)
const uint8_t SCAN_DATA_ADDRESS = 0x01;     // Data address sent during Modbus RTU Scan request (both attempts)
const uint16_t TCP_DISCON_TIMEOUT = 500;         // Timeout (ms) for client DISCON socket command, non-blocking alternative to https://www.arduino.cc/reference/en/libraries/ethernet/client.setconnectiontimeout/
const uint16_t WEB_IDLE_TIMEOUT = 400;           // Time (ms) from last client data after which webserver TCP socket could be disconnected, non-blocking.
const uint16_t FETCH_INTERVAL = 2000;            // Fetch API interval (ms) for the Modbus Status webpage to renew data from JSON served by Arduino

#endif /* PROJECTSETTINGS_H_ */
