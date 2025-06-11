/*
 * WebServer.hpp
 *
 *  Created on: Jun 6, 2025
 *      Author: Fki
 */

#ifndef WEBSERVER_HPP_
#define WEBSERVER_HPP_

#include "EthernetClient.h"
#include "StreamLib.h"

void recvWeb(EthernetClient &client);
void processPost(EthernetClient &client);
void sendPage(EthernetClient &client, uint8_t reqPage);
uint8_t strToByte(const char myStr[]);
void resetStats();
void clearQueue();
void jsonVal(ChunkedPrint &chunked, const uint8_t JSONKEY);
void stringPageName(ChunkedPrint &chunked, uint8_t item);
void stringStats(ChunkedPrint &chunked, const uint8_t stat);
void contentTools(ChunkedPrint &chunked);
void contentWait(ChunkedPrint &chunked);
void tagLabelDiv(ChunkedPrint &chunked, const char *label);
void tagDivClose(ChunkedPrint &chunked);
void tagButton(ChunkedPrint &chunked, const char *flashString, uint8_t value) ;
char *hex(uint8_t val);
void contentInfo(ChunkedPrint &chunked);
void contentStatus(ChunkedPrint &chunked);
void contentIp(ChunkedPrint &chunked);
void contentTcp(ChunkedPrint &chunked);
void contentRtu(ChunkedPrint &chunked);
void tagInputHex(ChunkedPrint &chunked, const uint8_t name, const bool required, const bool printVal, const uint8_t value);
void tagSpan(ChunkedPrint &chunked, const uint8_t JSONKEY);
void tagInputIp(ChunkedPrint &chunked, const uint8_t name, uint8_t ip[]);
void tagInputNumber(ChunkedPrint &chunked, const uint8_t name, const uint8_t min, uint16_t max, uint16_t value, const char *units);

#endif /* WEBSERVER_HPP_ */
