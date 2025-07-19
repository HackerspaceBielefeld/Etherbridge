/*
 * ModbusTCP.cpp
 *
 *  Created on: Jun 5, 2025
 *      Author: Fki
 */

#include "Board.hpp"
#include "Timer.h"
#include "ProjectSettings.h"
#include "Ethernet.h"
#include "main.h"
#include "utility/w5100.h"
#include "ModbusTCP.h"
#include "crc.hpp"
#include "SysTick.h"

uint8_t masks[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

uint32_t lastSocketUse[MAX_SOCK_NUM];
uint8_t socketInQueue[MAX_SOCK_NUM];

/**************************************************************************/
/*!
  @brief Initiates ethernet interface, if DHCP enabled, gets IP from DHCP,
  starts all servers (UDP, web server).
*/
/**************************************************************************/
void startEthernet() {
    Timer rstTimer;
    wzRstPin.clr();
    rstTimer.delay(25);
    wzRstPin.set();
    rstTimer.delay(ETH_RESET_DELAY);

  if (data.config.enableDhcp) {
    dhcpSuccess = Ethernet.begin(data.mac);
  }
  if (!data.config.enableDhcp || dhcpSuccess == false) {
    Ethernet.begin(data.mac, data.config.ip, data.config.dns, data.config.gateway, data.config.subnet);
  }
  W5100.setRetransmissionTime(TCP_RETRANSMISSION_TIMEOUT);
  W5100.setRetransmissionCount(TCP_RETRANSMISSION_COUNT);
  modbusServer = EthernetServer(data.config.tcpPort);
//  webServer = EthernetServer(data.config.webPort);
  Udp.begin(data.config.udpPort);
  modbusServer.begin();
//  webServer.begin();
#if MAX_SOCK_NUM > 4
  if (W5100.getChip() == 51) maxSockNum = 4;  // W5100 chip never supports more than 4 sockets
#endif
}

void recvTcp(EthernetClient &client) {
  uint16_t msgLength = client.available();
  uint8_t inBuffer[MODBUS_SIZE + 4];  // Modbus TCP frame is 4 bytes longer than Modbus RTU frame
                                   // Modbus TCP/UDP frame: [0][1] transaction ID, [2][3] protocol ID, [4][5] length and [6] unit ID (address).....
                                   // Modbus RTU frame: [0] address.....
  memset(inBuffer, 0, sizeof(inBuffer));
  client.read(inBuffer, sizeof(inBuffer));
  while (client.available()) client.read();
  uint8_t errorCode = checkRequest(inBuffer, msgLength, {}, client.remotePort(), TCP_REQUEST | client.getSocketNumber());
  if (errorCode) {
    // send back message with error code
    uint8_t i = 0;
    uint8_t outBuffer[9];
    if (!data.config.enableRtuOverTcp) {
      memcpy(outBuffer, inBuffer, 5);
      outBuffer[5] = 0x03;
      i = 6;
    }
    uint8_t addressPos = 6 * !data.config.enableRtuOverTcp;  // position of slave address in the incoming TCP/UDP message (0 for Modbus RTU over TCP/UDP and 6 for Modbus RTU over TCP/UDP)
    outBuffer[i++] = inBuffer[addressPos];                // address
    outBuffer[i++] = inBuffer[addressPos + 1] + 0x80;     // function + 0x80
    outBuffer[i++] = errorCode;
    if (data.config.enableRtuOverTcp) {
      crc = 0xFFFF;
      calculateCRC(inBuffer[addressPos]);
      calculateCRC(inBuffer[addressPos + 1] + 0x80);
      calculateCRC(errorCode);
      outBuffer[i++] = lowByte(crc);  // send CRC, low byte first
      outBuffer[i++] = highByte(crc);
    }
    client.write(outBuffer, i);
  }
}

void scanRequest() {
  // Insert scan request into queue, allow only one scan request in a queue
  static uint8_t scanCommand[] = { SCAN_FUNCTION_FIRST, 0x00, SCAN_DATA_ADDRESS, 0x00, 0x01 };
  if (scanCounter != 0 && queueHeaders.available() > 1 && queueData.available() > sizeof(scanCommand) + 1 && scanReqInQueue == false) {
    scanReqInQueue = true;
    // Store scan request in request queue
    queueHeaders.push(header_t{
      { 0x00, 0x00 },           // tid[2]
      sizeof(scanCommand) + 1,  // msgLen
      { 0, 0, 0, 0 },           // remIP
      0,                        // remPort
      SCAN_REQUEST,             // requestType
      0,                        // atts
    });
    queueData.push(scanCounter);  // address of the scanned slave
    for (uint8_t i = 0; i < sizeof(scanCommand); i++) {
      queueData.push(scanCommand[i]);
    }
    if (scanCommand[0] == SCAN_FUNCTION_FIRST) {
      scanCommand[0] = SCAN_FUNCTION_SECOND;
    } else {
      scanCommand[0] = SCAN_FUNCTION_FIRST;
      scanCounter++;
    }
    if (scanCounter == MAX_SLAVES + 1) scanCounter = 0;
  }
}

/**************************************************************************/
/*!
  @brief Receives Modbus UDP (or Modbus RTU over UDP) messages, calls @ref checkRequest()
*/
/**************************************************************************/
void recvUdp() {
  uint16_t msgLength = Udp.parsePacket();
  if (msgLength) {
    uint8_t inBuffer[MODBUS_SIZE + 4];  // Modbus TCP frame is 4 bytes longer than Modbus RTU frame
                                     // Modbus TCP/UDP frame: [0][1] transaction ID, [2][3] protocol ID, [4][5] length and [6] unit ID (address)..... no CRC
                                     // Modbus RTU frame: [0] address.....[n-1][n] CRC
    memset(inBuffer, 0, sizeof(inBuffer));
    Udp.read(inBuffer, sizeof(inBuffer));
    while (Udp.available()) Udp.read();
    uint8_t errorCode = checkRequest(inBuffer, msgLength, (uint32_t)Udp.remoteIP(), Udp.remotePort(), UDP_REQUEST);
    if (errorCode) {
      // send back message with error code
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      if (!data.config.enableRtuOverTcp) {
        Udp.write(inBuffer, 5);
        Udp.write(0x03);
      }
      uint8_t addressPos = 6 * !data.config.enableRtuOverTcp;  // position of slave address in the incoming TCP/UDP message (0 for Modbus RTU over TCP/UDP and 6 for Modbus RTU over TCP/UDP)
      Udp.write(inBuffer[addressPos]);                      // address
      Udp.write(inBuffer[addressPos + 1] + 0x80);           // function + 0x80
      Udp.write(errorCode);
      if (data.config.enableRtuOverTcp) {
        crc = 0xFFFF;
        calculateCRC(inBuffer[addressPos]);
        calculateCRC(inBuffer[addressPos + 1] + 0x80);
        calculateCRC(errorCode);
        Udp.write(lowByte(crc));  // send CRC, low byte first
        Udp.write(highByte(crc));
      }
      Udp.endPacket();
    }
  }
}

/**************************************************************************/
/*!
  @brief Checks Modbus TCP/UDP requests (correct MBAP header,
  CRC in case of Modbus RTU over TCP/UDP), checks availability of queue,
  stores requests into queue or returns an error.
  @param inBuffer Modbus TCP/UDP requests
  @param msgLength Length of the Modbus TCP/UDP requests
  @param remoteIP Remote IP
  @param remotePort Remote port
  @param requestType UDP or TCP, priority or scan request
  @return Modbus error code to be sent back to the recipient.
*/
/**************************************************************************/
uint8_t checkRequest(uint8_t inBuffer[], uint16_t msgLength, const uint32_t remoteIP, const uint16_t remotePort, uint8_t requestType) {
  uint8_t addressPos = 6 * !data.config.enableRtuOverTcp;  // position of slave address in the incoming TCP/UDP message (0 for Modbus RTU over TCP/UDP and 6 for Modbus RTU over TCP/UDP)
  if (data.config.enableRtuOverTcp) {                   // check CRC for Modbus RTU over TCP/UDP
    if (checkCRC(inBuffer, msgLength) == false) {
      data.errorCnt[ERROR_TCP]++;
      return 0;  // drop request and do not return any error code
    }
  } else {  // check MBAP header structure for Modbus TCP/UDP
    if (inBuffer[2] != 0x00 || inBuffer[3] != 0x00 || inBuffer[4] != 0x00 || inBuffer[5] != msgLength - 6) {
      data.errorCnt[ERROR_TCP]++;
      return 0;  // drop request and do not return any error code
    }
  }
  msgLength = msgLength - addressPos - (2 * data.config.enableRtuOverTcp);  // in Modbus RTU over TCP/UDP do not store CRC
  // check if we have space in request queue
  if (queueHeaders.available() < 1 || queueData.available() < msgLength) {
    setSlaveStatus(inBuffer[addressPos], SLAVE_ERROR_0A, true, false);
    return 0x0A;  // return Modbus error code 10 (Gateway Overloaded)
  }
  // allow only one request to non responding slaves
  if (getSlaveStatus(inBuffer[addressPos], SLAVE_ERROR_0B_QUEUE)) {
    data.errorCnt[SLAVE_ERROR_0B]++;
    return 0x0B;  // return Modbus error code 11 (Gateway Target Device Failed to Respond)
  } else if (getSlaveStatus(inBuffer[addressPos], SLAVE_ERROR_0B)) {
    setSlaveStatus(inBuffer[addressPos], SLAVE_ERROR_0B_QUEUE, true, false);
  } else {
    // Add PRIORITY_REQUEST flag to requests for responding slaves
    requestType = requestType | PRIORITY_REQUEST;
    priorityReqInQueue++;
  }
  if (inBuffer[addressPos] == 0x00) {          // Modbus Broadcast
    requestType = requestType | SCAN_REQUEST;  // Treat broadcast as scan (only one attempt, short timeout, do not expect response)
  }
  // all checkes passed OK, we can store the incoming data in request queue
  if (requestType & TCP_REQUEST) {
    socketInQueue[requestType & TCP_REQUEST_MASK]++;
  }
  // Store in request queue
  queueHeaders.push(header_t{
    { inBuffer[0], inBuffer[1] },  // tid[2] (ignored in Modbus RTU over TCP/UDP)
    uint8_t(msgLength),               // msgLen
    (IPAddress)remoteIP,           // remIP
    (unsigned int)remotePort,      // remPort
    uint8_t(requestType),             // requestType
    0,                             // atts
  });
  for (uint8_t i = 0; i < msgLength; i++) {
    queueData.push(inBuffer[i + addressPos]);
  }
  if (queueData.size() > queueDataSize) queueDataSize = queueData.size();
  if (queueHeaders.size() > queueHeadersSize) queueHeadersSize = queueHeaders.size();
  return 0;
}

void setSlaveStatus(const uint8_t slave, uint8_t status, const bool value, const bool isScan) {
  if (slave >= MAX_SLAVES || status > SLAVE_ERROR_0B_QUEUE) return;  // error
  if (value == 0) {
    slaveStatus[status][slave / 8] &= ~masks[slave & 7];
  } else {
    for (uint8_t i = 0; i <= SLAVE_ERROR_0B_QUEUE; i++) {
      slaveStatus[i][slave / 8] &= ~masks[slave & 7];  // set all other flags to false, SLAVE_ERROR_0B_QUEUE is the last slave status
    }
    slaveStatus[status][slave / 8] |= masks[slave & 7];
    if (status != SLAVE_ERROR_0B_QUEUE && isScan == false) data.errorCnt[status]++;  // there is no counter for SLAVE_ERROR_0B_QUEUE, ignor scans in statistics
  }
}

bool getSlaveStatus(const uint8_t slave, const uint8_t status) {
  if (slave >= MAX_SLAVES) return false;  // error
  return (slaveStatus[status][slave / 8] & masks[slave & 7]) > 0;
}

/**************************************************************************/
/*!
  @brief Closes sockets which are waiting to be closed or which refuse to close,
  forwards sockets with data available for further processing by the webserver,
  disconnects (closes) sockets which are too old (idle for too long), opens
  new sockets if needed (and if available).
  From https://github.com/SapientHetero/Ethernet/blob/master/src/socket.cpp
*/
/**************************************************************************/
void manageSockets() {
  uint32_t maxAge = 0;         // the 'age' of the socket in a 'disconnectable' state that was last used the longest time ago
  uint8_t oldest = MAX_SOCK_NUM;  // the socket number of the 'oldest' disconnectable socket
  uint8_t modbusListening = MAX_SOCK_NUM;
  uint8_t webListening = MAX_SOCK_NUM;
  uint8_t dataAvailable = MAX_SOCK_NUM;
  uint8_t socketsAvailable = 0;
  while(wzChannel.begin() != SUCCESS);  // begin SPI transaction
  // look at all the hardware sockets, record and take action based on current states
  for (uint8_t s = 0; s < maxSockNum; s++) {            // for each hardware socket ...
    uint8_t status = W5100.readSnSR(s);                 //  get socket status...
    uint32_t sockAge = SysTick_GetMillis() - lastSocketUse[s];  // age of the current socket
    if (socketInQueue[s] > 0) {
      lastSocketUse[s] = SysTick_GetMillis();
      continue;  // do not close Modbus TCP sockets currently processed (in queue)
    }

    switch (status) {
      case SnSR::CLOSED:
        {
          socketsAvailable++;
        }
        break;
      case SnSR::LISTEN:
      case SnSR::SYNRECV:
        {
          lastSocketUse[s] = SysTick_GetMillis();
          if (W5100.readSnPORT(s) == data.config.webPort) {
            webListening = s;
          } else {
            modbusListening = s;
          }
        }
        break;
      case SnSR::FIN_WAIT:
      case SnSR::CLOSING:
      case SnSR::TIME_WAIT:
      case SnSR::LAST_ACK:
        {
          socketsAvailable++;                  // socket will be available soon
          if (sockAge > TCP_DISCON_TIMEOUT) {  //     if it's been more than TCP_CLIENT_DISCON_TIMEOUT since disconnect command was sent...
            W5100.execCmdSn(s, Sock_CLOSE);    //       send CLOSE command...
            lastSocketUse[s] = SysTick_GetMillis();       //       and record time at which it was sent so we don't do it repeatedly.
          }
        }
        break;
      case SnSR::ESTABLISHED:
      case SnSR::CLOSE_WAIT:
        {
          if (EthernetClient(s).available() > 0) {
            dataAvailable = s;
            lastSocketUse[s] = SysTick_GetMillis();
          } else {
            // remote host closed connection, our end still open
            if (status == SnSR::CLOSE_WAIT) {
              socketsAvailable++;               // socket will be available soon
              W5100.execCmdSn(s, Sock_DISCON);  //  send DISCON command...
              lastSocketUse[s] = SysTick_GetMillis();      //   record time at which it was sent...
                                                // status becomes LAST_ACK for short time
            } else if (((W5100.readSnPORT(s) == data.config.webPort && sockAge > WEB_IDLE_TIMEOUT)
                        || (W5100.readSnPORT(s) == data.config.tcpPort && sockAge > (data.config.tcpTimeout * 1000UL)))
                       && sockAge > maxAge) {
              oldest = s;        //     record the socket number...
              maxAge = sockAge;  //      and make its age the new max age.
            }
          }
        }
        break;
      default:
        break;
    }
  }

  if (dataAvailable != MAX_SOCK_NUM) {
    EthernetClient client = EthernetClient(dataAvailable);
//    if (W5100.readSnPORT(dataAvailable) == data.config.webPort) {
//      recvWeb(client);
//    } else {
      recvTcp(client);
//    }
  }

  if (modbusListening == MAX_SOCK_NUM) {
    modbusServer.begin();
  }
//  else if (webListening == MAX_SOCK_NUM) {
//    webServer.begin();
//  }

  // If needed, disconnect socket that's been idle (ESTABLISHED without data recieved) the longest
  if (oldest != MAX_SOCK_NUM && socketsAvailable == 0 && (webListening == MAX_SOCK_NUM || modbusListening == MAX_SOCK_NUM)) {
    disconSocket(oldest);
  }

  while(wzChannel.end() != SUCCESS);  // Serves to o release the bus for other devices to access it. Since the ethernet chip is the only device
  // we do not need SPI.beginTransaction(SPI_ETHERNET_SETTINGS) or SPI.endTransaction() ??
  Ethernet.maintain(); //Manage DHCP
}

/**************************************************************************/
/*!
  @brief Disconnect or close a socket.
  @param s Socket number.
*/
/**************************************************************************/
void disconSocket(uint8_t s) {
  if (W5100.readSnSR(s) == SnSR::ESTABLISHED) {
    W5100.execCmdSn(s, Sock_DISCON);  // Sock_DISCON does not close LISTEN sockets
    lastSocketUse[s] = SysTick_GetMillis();      //   record time at which it was sent...
  } else {
    W5100.execCmdSn(s, Sock_CLOSE);  //  send DISCON command...
  }
}

void sendResponse(const uint8_t MBAP[], const uint8_t PDU[], const uint16_t pduLength) {
  header_t myHeader = queueHeaders.first();
  responseLen = 0;
  while (responseLen < pduLength) {  // include CRC
    if (responseLen < MAX_RESPONSE_LEN) {
      response[responseLen] = PDU[responseLen];
    }
    responseLen++;
  }
  if (myHeader.requestType & UDP_REQUEST) {
    Udp.beginPacket(myHeader.remIP, myHeader.remPort);
    if (data.config.enableRtuOverTcp) Udp.write(PDU, pduLength);
    else {
      Udp.write(MBAP, 6);
      Udp.write(PDU, pduLength - 2);  //send without CRC
    }
    Udp.endPacket();
  } else if (myHeader.requestType & TCP_REQUEST) {
      uint8_t sock = myHeader.requestType & TCP_REQUEST_MASK;
    EthernetClient client = EthernetClient(sock);
    if (W5100.readSnSR(sock) == SnSR::ESTABLISHED && W5100.readSnDPORT(sock) == myHeader.remPort) {  // Check remote port should be enough or check also rem IP?
      if (data.config.enableRtuOverTcp) client.write(PDU, pduLength);
      else {
        client.write(MBAP, 6);
        client.write(PDU, pduLength - 2);  //send without CRC
      }
    }  // TODO TCP Connection Error
  }    // else SCAN_REQUEST (no data.ethCnt[DATA_TX], but yes delete request)
  deleteRequest();
}

void deleteRequest()  // delete request from queue
{
  header_t myHeader = queueHeaders.first();
  if (myHeader.requestType & SCAN_REQUEST) scanReqInQueue = false;
  if (myHeader.requestType & TCP_REQUEST) socketInQueue[myHeader.requestType & TCP_REQUEST_MASK]--;
  if (myHeader.requestType & PRIORITY_REQUEST) priorityReqInQueue--;
  for (size_t i = 0; i < myHeader.msgLen; i++) {
    queueData.shift();
  }
  queueHeaders.shift();
}
