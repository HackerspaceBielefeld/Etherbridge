/*
 * WebServer.cpp
 *
 *  Created on: Jun 6, 2025
 *      Author: Fki
 */

#include <cstdint>
#include "WebServer.hpp"
#include "main.h"
#include "ModbusTCP.h"
#include "ProjectSettings.h"
#include "utility/w5100.h"
#include "ModbusTCP.h"

const uint8_t URI_SIZE = 24;   // a smaller buffer for uri
const uint8_t POST_SIZE = 24;  // a smaller buffer for single post parameter + key
const uint8_t WEB_OUT_BUFFER_SIZE = 64;  // size of web server write buffer (used by StreamLib)
const char VERSION[] = { '7', '3' };

// Actions that need to be taken after saving configuration.
enum action_type : uint8_t {
  ACT_NONE,
  ACT_DEFAULT,        // Load default factory settings (but keep MAC address)
  ACT_MAC,            // Generate new random MAC
  ACT_REBOOT,         // Reboot the microcontroller
  ACT_RESET_ETH,      // Ethernet reset
  ACT_RESET_SERIAL,   // Serial reset
  ACT_SCAN,           // Initialize RTU scan
  ACT_RESET_STATS,    // Reset Modbus Statistics
  ACT_CLEAR_REQUEST,  // Clear Modbus Request form
  ACT_WEB             // Restart webserver
};
enum action_type action;

// Pages served by the webserver. Order of elements defines the order in the left menu of the web UI.
// URL of the page (*.htm) contains number corresponding to its position in this array.
// The following enum array can have a maximum of 10 elements (incl. PAGE_NONE and PAGE_WAIT)
enum page : uint8_t {
  PAGE_ERROR,  // 404 Error
  PAGE_INFO,
  PAGE_STATUS,
  PAGE_IP,
  PAGE_TCP,
  PAGE_RTU,
  PAGE_TOOLS,
  PAGE_WAIT,  // page with "Reloading. Please wait..." message.
  PAGE_DATA,  // d.json
};

// Keys for POST parameters, used in web forms and processed by processPost() function.
// Using enum ensures unique identification of each POST parameter key and consistence across functions.
// In HTML code, each element will apear as number corresponding to its position in this array.
enum post_key : uint8_t {
  POST_NONE,  // reserved for NULL
  POST_DHCP,  // enable DHCP
  POST_MAC,
  POST_MAC_1,
  POST_MAC_2,
  POST_MAC_3,
  POST_MAC_4,
  POST_MAC_5,
  POST_IP,
  POST_IP_1,
  POST_IP_2,
  POST_IP_3,  // IP address         || Each part of an IP address has its own POST parameter.     ||
  POST_SUBNET,
  POST_SUBNET_1,
  POST_SUBNET_2,
  POST_SUBNET_3,  // subnet             || Because HTML code for IP, subnet, gateway and DNS          ||
  POST_GATEWAY,
  POST_GATEWAY_1,
  POST_GATEWAY_2,
  POST_GATEWAY_3,  // gateway            || is generated through one (nested) for-loop,                ||
  POST_DNS,
  POST_DNS_1,
  POST_DNS_2,
  POST_DNS_3,        // DNS                || all these 16 enum elements must be listed in succession!!  ||
  POST_TCP,          // TCP port                  || Because HTML code for these 3 ports              ||
  POST_UDP,          // UDP port                  || is generated through one for-loop,               ||
  POST_WEB,          // web UI port               || these 3 elements must be listed in succession!!  ||
  POST_RTU_OVER,     // RTU over TCP/UDP
  POST_TCP_TIMEOUT,  // Modbus TCP socket close timeout
  POST_BAUD,         // baud rate
  POST_DATA,         // data bits
  POST_PARITY,       // parity
  POST_STOP,         // stop bits
  POST_FRAMEDELAY,   //frame delay
  POST_TIMEOUT,      // response timeout
  POST_ATTEMPTS,     // number of request attempts
  POST_REQ,          // Modbus request send from WebUI (first byte)
  POST_REQ_1,
  POST_REQ_2,
  POST_REQ_3,
  POST_REQ_4,
  POST_REQ_5,
  POST_REQ_6,
  POST_REQ_LAST,  // 8 bytes in total
  POST_ACTION,    // actions on Tools page
};

// Keys for JSON elements, used in: 1) JSON documents, 2) ID of span tags, 3) Javascript.
enum JSON_type : uint8_t {
  JSON_TIME,  // Runtime seconds
  JSON_RTU_DATA,
  JSON_ETH_DATA,
  JSON_RESPONSE,
  JSON_STATS,  // Modbus statistics from array data.errorCnt[]
  JSON_QUEUE,
  JSON_TCP_UDP_MASTERS,  // list of Modbus TCP/UDP masters separated by <br>
  JSON_SLAVES,           // list of Modbus RTU slaves separated by <br>
  JSON_SOCKETS,
  JSON_LAST,  // Must be the very last element in this array
};

uint8_t request[POST_REQ_LAST - POST_REQ + 1];  // Array to store Modbus request sent from WebUI
uint8_t requestLen = 0;                         // Length of the Modbus request send from WebUI

/**************************************************************************/
/*!
  @brief Receives GET requests for web pages, receives POST data from web forms,
  calls @ref processPost() function, sends web pages. For simplicity, all web pages
  should are numbered (1.htm, 2.htm, ...), the page number is passed to
  the @ref sendPage() function. Also executes actions (such as ethernet restart,
  reboot) during "please wait" web page.
  @param client Ethernet TCP client.
*/
/**************************************************************************/
void recvWeb(EthernetClient &client) {
  char uri[URI_SIZE];  // the requested page
  memset(uri, 0, sizeof(uri));
  while (client.available()) {        // start reading the first line which should look like: GET /uri HTTP/1.1
    if (client.read() == ' ') break;  // find space before /uri
  }
  uint8_t len = 0;
  while (client.available() && len < sizeof(uri) - 1) {
    char c = client.read();  // parse uri
    if (c == ' ') break;     // find space after /uri
    uri[len] = c;
    len++;
  }
  while (client.available()) {
    if (client.read() == '\r')
      if (client.read() == '\n')
        if (client.read() == '\r')
          if (client.read() == '\n')
            break;  // find 2 end of lines between header and body
  }
  if (client.available()) {
    processPost(client);  // parse post parameters
  }

  // Get the requested page from URI
  uint8_t reqPage = PAGE_ERROR;  // requested page, 404 error is a default
  if (uri[0] == '/') {
    if (uri[1] == '\0')  // the homepage System Info
      reqPage = PAGE_INFO;
    else if (!strcmp(uri + 2, ".htm")) {
      reqPage = uint8_t(uri[1] - 48);  // Convert single ASCII char to byte
      if (reqPage >= PAGE_WAIT) reqPage = PAGE_ERROR;
    } else if (!strcmp(uri, "/d.json")) {
      reqPage = PAGE_DATA;
    }
  }
  // Actions that require "please wait" page
  if (action == ACT_WEB || action == ACT_MAC || action == ACT_RESET_ETH || action == ACT_REBOOT || action == ACT_DEFAULT) {
    reqPage = PAGE_WAIT;
  }
  // Send page
  sendPage(client, reqPage);

  // Do all actions before the "please wait" redirects (5s delay at the moment)
  if (reqPage == PAGE_WAIT) {
    switch (action) {
      case ACT_WEB:
        for (uint8_t s = 0; s < maxSockNum; s++) {
          // close old webserver TCP connections
          if (EthernetClient(s).localPort() != data.config.tcpPort) {
            disconSocket(s);
          }
        }
        webServer = EthernetServer(data.config.webPort);
        break;
      case ACT_MAC:
      case ACT_RESET_ETH:
        for (uint8_t s = 0; s < maxSockNum; s++) {
          // close all TCP and UDP sockets
          disconSocket(s);
        }
        startEthernet();
        break;
      case ACT_REBOOT:
      case ACT_DEFAULT:
//        resetFunc();
        break;
      default:
        break;
    }
  }
  action = ACT_NONE;
}

/**************************************************************************/
/*!
  @brief Processes POST data from forms and buttons, updates data.config (in RAM)
  and saves config into EEPROM. Executes actions which do not require webserver restart
  @param client Ethernet TCP client.
*/
/**************************************************************************/
void processPost(EthernetClient &client) {
  while (client.available()) {
    char post[POST_SIZE];
    uint8_t len = 0;
    while (client.available() && len < sizeof(post) - 1) {
      char c = client.read();
      if (c == '&') break;
      post[len] = c;
      len++;
    }
    post[len] = '\0';
    char *paramKey = post;
    char *paramValue = post;
    while (*paramValue) {
      if (*paramValue == '=') {
        paramValue++;
        break;
      }
      paramValue++;
    }
    if (*paramValue == '\0')
      continue;  // do not process POST parameter if there is no parameter value
    uint8_t paramKeyByte = strToByte(paramKey);
    uint16_t paramValueUint = atol(paramValue);
    switch (paramKeyByte) {
      case POST_NONE:  // reserved, because atoi / atol returns NULL in case of error
        break;
#ifdef ENABLE_DHCP
      case POST_DHCP:
        {
          data.config.enableDhcp = uint8_t(paramValueUint);
        }
        break;
      case POST_DNS ... POST_DNS_3:
        {
          data.config.dns[paramKeyByte - POST_DNS] = uint8_t(paramValueUint);
        }
        break;
#endif /* ENABLE_DHCP */
      case POST_REQ ... POST_REQ_LAST:
        {
          requestLen = paramKeyByte - POST_REQ + 1;
          request[requestLen - 1] = strToByte(paramValue);
        }
        break;
      case POST_MAC ... POST_MAC_5:
        {
          action = ACT_RESET_ETH;  // this RESET_ETH is triggered when the user changes anything on the "IP Settings" page.
                                   // No need to trigger RESET_ETH for other cases (POST_SUBNET, POST_GATEWAY etc.)
                                   // if "Randomize" button is pressed, action is set to ACT_MAC
          data.mac[paramKeyByte - POST_MAC] = strToByte(paramValue);
        }
        break;
      case POST_IP ... POST_IP_3:
        {
          data.config.ip[paramKeyByte - POST_IP] = uint8_t(paramValueUint);
        }
        break;
      case POST_SUBNET ... POST_SUBNET_3:
        {
          data.config.subnet[paramKeyByte - POST_SUBNET] = uint8_t(paramValueUint);
        }
        break;
      case POST_GATEWAY ... POST_GATEWAY_3:
        {
          data.config.gateway[paramKeyByte - POST_GATEWAY] = uint8_t(paramValueUint);
        }
        break;
      case POST_TCP:
        {
          if (paramValueUint != data.config.webPort && paramValueUint != data.config.tcpPort) {  // continue only of the value changed and it differs from WebUI port
            for (uint8_t s = 0; s < maxSockNum; s++) {
              if (EthernetClient(s).localPort() == data.config.tcpPort) {  // close only Modbus TCP sockets
                disconSocket(s);
              }
            }
            data.config.tcpPort = paramValueUint;
            modbusServer = EthernetServer(data.config.tcpPort);
          }
        }
        break;
      case POST_UDP:
        {
          data.config.udpPort = paramValueUint;
          Udp.stop();
          Udp.begin(data.config.udpPort);
        }
        break;
      case POST_WEB:
        {
          if (paramValueUint != data.config.webPort && paramValueUint != data.config.tcpPort) {  // continue only of the value changed and it differs from Modbus TCP port
            data.config.webPort = paramValueUint;
            action = ACT_WEB;
          }
        }
        break;
      case POST_RTU_OVER:
        data.config.enableRtuOverTcp = uint8_t(paramValueUint);
        break;
      case POST_TCP_TIMEOUT:
        data.config.tcpTimeout = paramValueUint;
        break;
      case POST_BAUD:
        {
//          action = ACT_RESET_SERIAL;  // this RESET_SERIAL is triggered when the user changes anything on the "RTU Settings" page.
//          // No need to trigger RESET_ETH for other cases (POST_DATA, POST_PARITY etc.)
//          data.config.baud = paramValueUint;
//          uint8_t minFrameDelay = uint8_t((frameDelay() / 1000UL) + 1);
//          if (data.config.frameDelay < minFrameDelay) {
//            data.config.frameDelay = minFrameDelay;
//          }
        }
        break;
      case POST_DATA:
        {
//          data.config.serialConfig = (data.config.serialConfig & 0xF9) | ((uint8_t(paramValueUint) - 5) << 1);
        }
        break;
      case POST_PARITY:
        {
//          data.config.serialConfig = (data.config.serialConfig & 0xCF) | (uint8_t(paramValueUint) << 4);
        }
        break;
      case POST_STOP:
        {
//          data.config.serialConfig = (data.config.serialConfig & 0xF7) | ((uint8_t(paramValueUint) - 1) << 3);
        }
        break;
      case POST_FRAMEDELAY:
        data.config.frameDelay = uint8_t(paramValueUint);
        break;
      case POST_TIMEOUT:
        data.config.serialTimeout = paramValueUint;
        break;
      case POST_ATTEMPTS:
        data.config.serialAttempts = uint8_t(paramValueUint);
        break;
      case POST_ACTION:
        action = action_type(paramValueUint);
        break;
      default:
        break;
    }
  }
  switch (action) {
    case ACT_DEFAULT:
      data.config = DEFAULT_CONFIG;
      break;
    case ACT_RESET_STATS:
      resetStats();
      break;
    case ACT_MAC:
//      generateMac();
      break;
    case ACT_RESET_SERIAL:
      clearQueue();
//      startSerial();
      break;
    case ACT_SCAN:
      scanCounter = 1;
      memset(&slaveStatus, 0, sizeof(slaveStatus));  // clear all status flags
      break;
    case ACT_CLEAR_REQUEST:
      requestLen = 0;
      responseLen = 0;
      break;
    default:
      break;
  }
  // if new Modbus request received, put into queue
  if (action != ACT_SCAN && action != ACT_RESET_STATS && requestLen > 1 && queueHeaders.available() > 1 && queueData.available() > requestLen) {  // at least 2 bytes in request (slave address and function)
    // push to queue
    queueHeaders.push(header_t{
      { 0x00, 0x00 },  // tid[2]
      requestLen,      // msgLen
      { 0, 0, 0, 0 },  // remIP[4]
      0,               // remPort
      UDP_REQUEST,     // requestType
      0,               // atts
    });
    for (uint8_t i = 0; i < requestLen; i++) {
      queueData.push(request[i]);
    }
    responseLen = 0;  // clear old Modbus Response from WebUI
  }
  // new parameter values received, save them to EEPROM
//  updateEeprom();  // it is safe to call, only changed values (and changed error and data counters) are updated
}

/**************************************************************************/
/*!
  @brief Sends the requested page (incl. 404 error and JSON document),
  displays main page, renders title and left menu using, calls content functions
  depending on the number (i.e. URL) of the requested web page.
  In order to save flash memory, some HTML closing tags are omitted,
  new lines in HTML code are also omitted.
  @param client Ethernet TCP client
  @param reqPage Requested page number
*/
/**************************************************************************/
void sendPage(EthernetClient &client, uint8_t reqPage) {
  char webOutBuffer[WEB_OUT_BUFFER_SIZE];
  ChunkedPrint chunked(client, webOutBuffer, sizeof(webOutBuffer));  // the StreamLib object to replace client print
  if (reqPage == PAGE_ERROR) {
    chunked.print(("HTTP/1.1 404 Not Found\r\n"
                    "\r\n"
                    "404 Not found"));
    chunked.end();
    return;
  } else if (reqPage == PAGE_DATA) {
    chunked.print(("HTTP/1.1 200\r\n"  // An advantage of HTTP 1.1 is that you can keep the connection alive
                    "Content-Type: application/json\r\n"
                    "Transfer-Encoding: chunked\r\n"
                    "\r\n"));
    chunked.begin();
    chunked.print(("{"));
    for (uint8_t i = 0; i < JSON_LAST; i++) {
      if (i) chunked.print((","));
      chunked.print(("\""));
      chunked.print(i);
      chunked.print(("\":\""));
      jsonVal(chunked, i);
      chunked.print(("\""));
    }
    chunked.print(("}"));
    chunked.end();
    return;
  }
  chunked.print(("HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html\r\n"
                  "Transfer-Encoding: chunked\r\n"
                  "\r\n"));
  chunked.begin();
  chunked.print(("<!DOCTYPE html>"
                  "<html>"
                  "<head>"
                  "<meta"));
  if (reqPage == PAGE_WAIT) {  // redirect to new IP and web port
    chunked.print((" http-equiv=refresh content=5;url=http://"));
    chunked.print(IPAddress(data.config.ip));
    chunked.print((":"));
    chunked.print(data.config.webPort);
  }
  chunked.print((">"
                  "<title>Modbus RTU &rArr; Modbus TCP/UDP Gateway</title>"
                  "<style>"
                  /*
                  HTML Tags
                    h1 - main title of the page
                    h4 - text in navigation menu and header of page content
                    a - items in left navigation menu
                    label - first cell of a row in content
                  CSS Classes
                    w - wrapper (includes m + c)
                    m  - navigation menu (left)
                    c - content of a page
                    r - row inside a content
                    i - short input (byte or IP address octet)
                    n - input type=number
                    s - select input with numbers
                    p - inputs disabled by id=o checkbox
                  CSS Ids
                    o - checkbox which disables other checkboxes and inputs
                  */
                  "body,.m{padding:1px;margin:0;font-family:sans-serif}"
                  "h1,h4{padding:10px}"
                  "h1,.m,h4{background:#0067AC;margin:1px}"
                  ".m,.c{height:calc(100vh - 71px)}"
                  ".m{min-width:20%}"
                  ".c{flex-grow:1;overflow-y:auto}"
                  ".w,.r{display:flex}"
                  "a,h1,h4{color:white;text-decoration:none}"
                  ".c h4{padding-left:30%;margin-bottom:20px}"
                  ".r{margin:4px}"
                  "label{width:30%;text-align:right;margin-right:2px}"
                  "input,button,select{margin-top:-2px}"  // improve vertical allignment of input, button and select
                  ".s{text-align:right}"
                  ".s>option{direction:rtl}"
                  ".i{text-align:center;width:3ch;color:black}"
                  ".n{width:8ch}"
                  "</style>"
                  "</head>"
                  "<body"));
#ifdef ENABLE_DHCP
  chunked.print(F(" onload=g(document.getElementById('o').checked)>"
                  "<script>function g(h) {var x = document.getElementsByClassName('p');for (var i = 0; i < x.length; i++) {x[i].disabled = h}}</script"));
#endif /* ENABLE_DHCP */
  if (reqPage == PAGE_STATUS) {
    chunked.print(("><script>"
                    "var a;"
                    "const b=()=>{"
                    "fetch('d.json')"  // Call the fetch function passing the url of the API as a parameter
                    ".then(e=>{return e.json();a=0})"
                    ".then(f=>{for(var i in f){if(document.getElementById(i))document.getElementById(i).innerHTML=f[i];}})"
                    ".catch(()=>{if(!a){alert('Connnection lost');a=1}})"
                    "};"
                    "setInterval(()=>b(),"));
    chunked.print(FETCH_INTERVAL);
    chunked.print((");"
                    "</script"));
  }
  chunked.print((">"
                  "<h1>Modbus RTU &rArr; Modbus TCP/UDP Gateway</h1>"
                  "<div class=w>"
                  "<div class=m>"));

  // Left Menu
  for (uint8_t i = 1; i < PAGE_WAIT; i++) {  // PAGE_WAIT is the last item in enum
    chunked.print(("<h4 "));
    if ((i) == reqPage) {
      chunked.print((" style=background-color:#FF6600"));
    }
    chunked.print(("><a href="));
    chunked.print(i);
    chunked.print((".htm>"));
    stringPageName(chunked, i);
    chunked.print(("</a></h4>"));
  }
  chunked.print(("</div>"  // <div class=w>
                  "<div class=c>"
                  "<h4>"));
  stringPageName(chunked, reqPage);
  chunked.print(("</h4>"
                  "<form method=post>"));

  //   PLACE FUNCTIONS PROVIDING CONTENT HERE
  switch (reqPage) {
    case PAGE_INFO:
      contentInfo(chunked);
      break;
    case PAGE_STATUS:
      contentStatus(chunked);
      break;
    case PAGE_IP:
      contentIp(chunked);
      break;
    case PAGE_TCP:
      contentTcp(chunked);
      break;
    case PAGE_RTU:
      contentRtu(chunked);
      break;
    case PAGE_TOOLS:
      contentTools(chunked);
      break;
    case PAGE_WAIT:
      contentWait(chunked);
      break;
    default:
      break;
  }

  if (reqPage == PAGE_IP || reqPage == PAGE_TCP || reqPage == PAGE_RTU) {
    chunked.print(("<p><div class=r><label><input type=submit value='Save & Apply'></label><input type=reset value=Cancel></div>"));
  }
  chunked.print(("</form>"));
  tagDivClose(chunked);  // close tags <div class=c> <div class=w>
  chunked.end();         // closing tags not required </body></html>
}

/**************************************************************************/
/*!
  @brief Parses string and returns single byte.
  @param myStr String (2 chars, 1 char + null or 1 null) to be parsed.
  @return Parsed byte.
*/
/**************************************************************************/
uint8_t strToByte(const char myStr[]) {
  if (!myStr) return 0;
  uint8_t x = 0;
  for (uint8_t i = 0; i < 2; i++) {
    char c = myStr[i];
    if (c >= '0' && c <= '9') {
      x *= 16;
      x += c - '0';
    } else if (c >= 'A' && c <= 'F') {
      x *= 16;
      x += (c - 'A') + 10;
    } else if (c >= 'a' && c <= 'f') {
      x *= 16;
      x += (c - 'a') + 10;
    }
  }
  return x;
}

/**************************************************************************/
/*!
  @brief Resets error stats, RTU counter and ethernet data counter.
*/
/**************************************************************************/
void resetStats() {
  memset(data.errorCnt, 0, sizeof(data.errorCnt));
#ifdef ENABLE_EXTENDED_WEBUI
  memset(data.rtuCnt, 0, sizeof(data.rtuCnt));
  memset(data.ethCnt, 0, sizeof(data.ethCnt));
  remaining_seconds = -(millis() / 1000);
#endif /* ENABLE_EXTENDED_WEBUI */
}

void clearQueue() {
  queueHeaders.clear();
  queueData.clear();
  scanReqInQueue = false;
  priorityReqInQueue = false;
  memset(socketInQueue, 0, sizeof(socketInQueue));
  memset(slaveStatus[SLAVE_ERROR_0B_QUEUE], 0, sizeof(slaveStatus[SLAVE_ERROR_0B_QUEUE]));
//  sendMicroTimer.sleep(0);
}

/**************************************************************************/
/*!
  @brief Provide JSON value to a corresponding JSON key. The value is printed
  in <span> and in JSON document fetched on the background.
  @param chunked Chunked buffer
  @param JSONKEY JSON key
*/
/**************************************************************************/
void jsonVal(ChunkedPrint &chunked, const uint8_t JSONKEY) {
  switch (JSONKEY) {
#ifdef ENABLE_EXTENDED_WEBUI
    case JSON_TIME:
      chunked.print(seconds / (3600UL * 24L));
      chunked.print((" days, "));
      chunked.print((seconds / 3600UL) % 24L);
      chunked.print((" hours, "));
      chunked.print((seconds / 60UL) % 60L);
      chunked.print((" mins, "));
      chunked.print((seconds) % 60L);
      chunked.print((" secs"));
      break;
    case JSON_RTU_DATA:
      for (uint8_t i = 0; i < DATA_LAST; i++) {
        chunked.print(data.rtuCnt[i]);
        switch (i) {
          case DATA_TX:
            chunked.print((" Tx bytes / "));
            break;
          case DATA_RX:
            chunked.print((" Rx bytes"));
            break;
        }
      }
    case JSON_ETH_DATA:
      for (uint8_t i = 0; i < DATA_LAST; i++) {
        chunked.print(data.ethCnt[i]);
        switch (i) {
          case DATA_TX:
            chunked.print((" Tx bytes / "));
            break;
          case DATA_RX:
            chunked.print((" Rx bytes (excl. WebUI)"));
            break;
        }
      }
      break;
#endif /* ENABLE_EXTENDED_WEBUI */
    case JSON_RESPONSE:
      {
        for (uint8_t i = 0; i < MAX_RESPONSE_LEN; i++) {
          chunked.print(("<input value='"));
          if (i < responseLen) {
            chunked.print(hex(response[i]));
          }
          chunked.print(("' disabled class=i>"));
        }
        chunked.print(("h"));
        if (responseLen > MAX_RESPONSE_LEN) {
          chunked.print((" +"));
          chunked.print(uint8_t(responseLen - MAX_RESPONSE_LEN));
          chunked.print((" bytes"));
        }
      }
      break;
    case JSON_QUEUE:
      chunked.print(queueDataSize);
      chunked.print((" / "));
      chunked.print(MAX_QUEUE_DATA);
      chunked.print((" bytes<br>"));
      chunked.print(queueHeadersSize);
      chunked.print((" / "));
      chunked.print(MAX_QUEUE_REQUESTS);
      chunked.print((" requests"));
      queueDataSize = queueData.size();
      queueHeadersSize = queueHeaders.size();
      break;
    case JSON_STATS:
      for (uint8_t i = 0; i < ERROR_LAST; i++) {
        if (i == SLAVE_ERROR_0B_QUEUE) continue;  // SLAVE_ERROR_0B_QUEUE is not shown in web UI
        chunked.print(data.errorCnt[i]);
        stringStats(chunked, i);
      }
      break;
    case JSON_TCP_UDP_MASTERS:
      {
        for (uint8_t s = 0; s < maxSockNum; s++) {
          uint8_t remoteIParray[4];
          W5100.readSnDIPR(s, remoteIParray);
          if (remoteIParray[0] != 0) {
            if (W5100.readSnSR(s) == SnSR::UDP) {
              chunked.print(IPAddress(remoteIParray));
              chunked.print((" UDP<br>"));
            } else if (W5100.readSnSR(s) == SnSR::ESTABLISHED && W5100.readSnPORT(s) == data.config.tcpPort) {
              chunked.print(IPAddress(remoteIParray));
              chunked.print((" TCP<br>"));
            }
          }
        }
      }
      break;
    case JSON_SLAVES:
      {
        for (uint8_t k = 1; k < MAX_SLAVES; k++) {
          for (uint8_t s = 0; s <= SLAVE_ERROR_0B_QUEUE; s++) {
            if (getSlaveStatus(k, s) == true || k == scanCounter) {
              chunked.print(hex(k));
              chunked.print(("h"));
              if (k == scanCounter) {
                chunked.print((" Scanning...<br>"));
                break;
              }
              stringStats(chunked, s);
            }
          }
        }
      }
      break;
    default:
      break;
  }
}

/**************************************************************************/
/*!
  @brief Menu item strings

  @param chunked Chunked buffer
  @param item Page number
*/
/**************************************************************************/
void stringPageName(ChunkedPrint &chunked, uint8_t item) {
  switch (item) {
    case PAGE_INFO:
      chunked.print(("System Info"));
      break;
    case PAGE_STATUS:
      chunked.print(("Modbus Status"));
      break;
    case PAGE_IP:
      chunked.print(("IP Settings"));
      break;
    case PAGE_TCP:
      chunked.print(("TCP/UDP Settings"));
      break;
    case PAGE_RTU:
      chunked.print(("RTU Settings"));
      break;
    case PAGE_TOOLS:
      chunked.print(("Tools"));
      break;
    default:
      break;
  }
}

void stringStats(ChunkedPrint &chunked, const uint8_t stat) {
  switch (stat) {
    case SLAVE_OK:
      chunked.print((" Slave Responded"));
      break;
    case SLAVE_ERROR_0X:
      chunked.print((" Slave Responded with Error (Codes 1~8)"));
      break;
    case SLAVE_ERROR_0A:
      chunked.print((" Gateway Overloaded (Code 10)"));
      break;
    case SLAVE_ERROR_0B:
    case SLAVE_ERROR_0B_QUEUE:
      chunked.print((" Slave Failed to Respond (Code 11)"));
      break;
    case ERROR_TIMEOUT:
      chunked.print((" Response Timeout"));
      break;
    case ERROR_RTU:
      chunked.print((" Invalid RTU Response"));
      break;
    case ERROR_TCP:
      chunked.print((" Invalid TCP/UDP Request"));
      break;
    default:
      break;
  }
  chunked.print(("<br>"));
}

/**************************************************************************/
/*!
  @brief Tools

  @param chunked Chunked buffer
*/
/**************************************************************************/
void contentTools(ChunkedPrint &chunked) {
  tagLabelDiv(chunked, 0);
  tagButton(chunked, ("Load Default Settings"), ACT_DEFAULT);
  chunked.print((" (static IP: "));
  chunked.print(IPAddress(DEFAULT_CONFIG.ip));
  chunked.print((")"));
  tagDivClose(chunked);
  tagLabelDiv(chunked, 0);
  tagButton(chunked, ("Reboot"), ACT_REBOOT);
  tagDivClose(chunked);
}


void contentWait(ChunkedPrint &chunked) {
  tagLabelDiv(chunked, 0);
  chunked.print(("Reloading. Please wait..."));
  tagDivClose(chunked);
}

/**************************************************************************/
/*!
  @brief <label><div>

  @param chunked Chunked buffer
  @param label Label string
*/
/**************************************************************************/
void tagLabelDiv(ChunkedPrint &chunked, const char *label) {
  chunked.print(("<div class=r>"));
  chunked.print(("<label> "));
  if (label) {
    chunked.print(label);
    chunked.print((":"));
  }
  chunked.print(("</label><div>"));
}

/**************************************************************************/
/*!
  @brief </div>

  @param chunked Chunked buffer
*/
/**************************************************************************/
void tagDivClose(ChunkedPrint &chunked) {
  chunked.print(("</div>"
                  "</div>"));  // <div class=r>
}

/**************************************************************************/
/*!
  @brief <button>

  @param chunked Chunked buffer
  @param flashString Button string
  @param value Value to be sent via POST
*/
/**************************************************************************/
void tagButton(ChunkedPrint &chunked, const char *flashString, uint8_t value) {
  chunked.print((" <button name="));
  chunked.print(POST_ACTION, HEX);
  chunked.print((" value="));
  chunked.print(value);
  chunked.print((">"));
  chunked.print(flashString);
  chunked.print(("</button>"));
}

/**************************************************************************/
/*!
  @brief Converts byte to char string, from https://github.com/RobTillaart/printHelpers
  @param val Byte to be conferted.
  @return Char string.
*/
/**************************************************************************/
char __printbuffer[3];

char *hex(uint8_t val) {
  char *buffer = __printbuffer;
  uint8_t digits = 2;
  buffer[digits] = '\0';
  while (digits > 0) {
    uint8_t v = val & 0x0F;
    val >>= 4;
    digits--;
    buffer[digits] = (v < 10) ? '0' + v : ('A' - 10) + v;
  }
  return buffer;
}

/**************************************************************************/
/*!
  @brief System Info

  @param chunked Chunked buffer
*/
/**************************************************************************/
void contentInfo(ChunkedPrint &chunked) {
  tagLabelDiv(chunked, ("SW Version"));
  chunked.print(VERSION[0]);
  chunked.print(("."));
  chunked.print(VERSION[1]);
  tagDivClose(chunked);
//  tagLabelDiv(chunked, ("Microcontroller"));
//  chunked.print(BOARD);
//  tagDivClose(chunked);
  tagLabelDiv(chunked, ("EEPROM Health"));
  chunked.print(data.eepromWrites);
  chunked.print((" Write Cycles"));
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("Ethernet Chip"));
  switch (W5100.getChip()) {
    case 51:
      chunked.print(("W5100"));
      break;
    case 52:
      chunked.print(("W5200"));
      break;
    case 55:
      chunked.print(("W5500"));
      break;
    default:  // TODO: add W6100 once it is included in Ethernet library
      chunked.print(("Unknown"));
      break;
  }
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("Ethernet Sockets"));
  chunked.print(maxSockNum);
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("MAC Address"));
  for (uint8_t i = 0; i < 6; i++) {
    chunked.print(hex(data.mac[i]));
    if (i < 5) chunked.print((":"));
  }
  tagDivClose(chunked);

#ifdef ENABLE_DHCP
  tagLabelDiv(chunked, ("DHCP Status"));
  if (!data.config.enableDhcp) {
    chunked.print(("Disabled"));
  } else if (dhcpSuccess == true) {
    chunked.print(("Success"));
  } else {
    chunked.print(("Failed, using fallback static IP"));
  }
  tagDivClose(chunked);
#endif /* ENABLE_DHCP */

  tagLabelDiv(chunked, ("IP Address"));
  chunked.print(IPAddress(Ethernet.localIP()));
  tagDivClose(chunked);
}

/**************************************************************************/
/*!
  @brief P1P2 Status

  @param chunked Chunked buffer
*/
/**************************************************************************/
void contentStatus(ChunkedPrint &chunked) {

#ifdef ENABLE_EXTENDED_WEBUI
  tagLabelDiv(chunked, ("Run Time"));
  tagSpan(chunked, JSON_TIME);
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("RTU Data"));
  tagSpan(chunked, JSON_RTU_DATA);
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("Ethernet Data"));
  tagSpan(chunked, JSON_ETH_DATA);
  tagDivClose(chunked);
#endif /* ENABLE_EXTENDED_WEBUI */

  tagLabelDiv(chunked, ("Modbus RTU Request"));
  for (uint8_t i = 0; i <= POST_REQ_LAST - POST_REQ; i++) {
    bool required = false;
    bool printVal = false;
    uint8_t value = 0;
    if (i == 0 || i == 1) {
      required = true;  // first byte (slave address) and second byte (function code) are required
    }
    if (i < requestLen) {
      printVal = true;
      value = request[i];
    }
    tagInputHex(chunked, POST_REQ + i, required, printVal, value);
  }
  chunked.print(("h (without CRC) <input type=submit value=Send>"));
  tagButton(chunked, ("Clear"), ACT_CLEAR_REQUEST);
  tagDivClose(chunked);
  chunked.print(("</form><form method=post>"));
  tagLabelDiv(chunked, ("Modbus RTU Response"));
  tagSpan(chunked, JSON_RESPONSE);
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("Requests Queue"));
  tagSpan(chunked, JSON_QUEUE);
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("Modbus Statistics"));
  tagButton(chunked, ("Reset Stats"), ACT_RESET_STATS);
  chunked.print(("<br>"));
  tagSpan(chunked, JSON_STATS);
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("Modbus Masters"));
  tagSpan(chunked, JSON_TCP_UDP_MASTERS);
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("Modbus Slaves"));
  tagButton(chunked, ("Scan Slaves"), ACT_SCAN);
  chunked.print(("<br>"));
  tagSpan(chunked, JSON_SLAVES);
  tagDivClose(chunked);
}

/**************************************************************************/
/*!
  @brief IP Settings

  @param chunked Chunked buffer
*/
/**************************************************************************/
void contentIp(ChunkedPrint &chunked) {

  tagLabelDiv(chunked, ("MAC Address"));
  for (uint8_t i = 0; i < 6; i++) {
    tagInputHex(chunked, POST_MAC + i, true, true, data.mac[i]);
    if (i < 5) chunked.print((":"));
  }
  tagButton(chunked, ("Randomize"), ACT_MAC);
  tagDivClose(chunked);

#ifdef ENABLE_DHCP
  tagLabelDiv(chunked, F("Auto IP"));
  chunked.print(F("<input type=hidden name="));
  chunked.print(POST_DHCP, HEX);
  chunked.print(F(" value=0>"
                  "<input type=checkbox id=o name="));
  chunked.print(POST_DHCP, HEX);
  chunked.print(F(" onclick=g(this.checked) value=1"));
  if (data.config.enableDhcp) chunked.print(F(" checked"));
  chunked.print(F("> DHCP"));
  tagDivClose(chunked);
#endif /* ENABLE_DHCP */

  uint8_t *tempIp;
  for (uint8_t j = 0; j < 3; j++) {
    switch (j) {
      case 0:
        tagLabelDiv(chunked, ("Static IP"));
        tempIp = data.config.ip;
        break;
      case 1:
        tagLabelDiv(chunked, ("Submask"));
        tempIp = data.config.subnet;
        break;
      case 2:
        tagLabelDiv(chunked, ("Gateway"));
        tempIp = data.config.gateway;
        break;
      default:
        break;
    }
    tagInputIp(chunked, POST_IP + (j * 4), tempIp);
    tagDivClose(chunked);
  }
#ifdef ENABLE_DHCP
  tagLabelDiv(chunked, ("DNS Server"));
  tagInputIp(chunked, POST_DNS, data.config.dns);
  tagDivClose(chunked);
#endif /* ENABLE_DHCP */
}

/**************************************************************************/
/*!
  @brief TCP/UDP Settings

  @param chunked Chunked buffer
*/
/**************************************************************************/
void contentTcp(ChunkedPrint &chunked) {
  uint16_t value;
  for (uint8_t i = 0; i < 3; i++) {
    switch (i) {
      case 0:
        tagLabelDiv(chunked, ("Modbus TCP Port"));
        value = data.config.tcpPort;
        break;
      case 1:
        tagLabelDiv(chunked, ("Modbus UDP Port"));
        value = data.config.udpPort;
        break;
      case 2:
        tagLabelDiv(chunked, ("WebUI Port"));
        value = data.config.webPort;
        break;
      default:
        break;
    }
    tagInputNumber(chunked, POST_TCP + i, 1, 65535, value, (""));
    tagDivClose(chunked);
  }
  tagLabelDiv(chunked, ("Modbus Mode"));
  chunked.print(("<select name="));
  chunked.print(POST_RTU_OVER, HEX);
  chunked.print((">"));
  for (uint8_t i = 0; i < 2; i++) {
    chunked.print(("<option value="));
    chunked.print(i);
    if (data.config.enableRtuOverTcp == i) chunked.print((" selected"));
    chunked.print((">"));
    switch (i) {
      case 0:
        chunked.print(("Modbus TCP/UDP"));
        break;
      case 1:
        chunked.print(("Modbus RTU over TCP/UDP"));
        break;
      default:
        break;
    }
    chunked.print(("</option>"));
  }
  chunked.print(("</select>"));
  tagDivClose(chunked);
  tagLabelDiv(chunked, ("Modbus TCP Idle Timeout"));
  tagInputNumber(chunked, POST_TCP_TIMEOUT, 1, 3600, data.config.tcpTimeout, ("sec"));
  tagDivClose(chunked);
}

/**************************************************************************/
/*!
  @brief RTU Settings

  @param chunked Chunked buffer
*/
/**************************************************************************/
void contentRtu(ChunkedPrint &chunked) {
//  tagLabelDiv(chunked, F("Baud Rate"));
//  chunked.print(F("<select class=s name="));
//  chunked.print(POST_BAUD, HEX);
//  chunked.print(F(">"));
//  for (byte i = 0; i < (sizeof(BAUD_RATES) / 2); i++) {
//    chunked.print(F("<option value="));
//    chunked.print(BAUD_RATES[i]);
//    if (data.config.baud == BAUD_RATES[i]) chunked.print(F(" selected"));
//    chunked.print(F(">"));
//    chunked.print(BAUD_RATES[i]);
//    chunked.print(F("00</option>"));
//  }
//  chunked.print(F("</select> bps"));
//  tagDivClose(chunked);
//  tagLabelDiv(chunked, F("Data Bits"));
//  chunked.print(F("<select name="));
//  chunked.print(POST_DATA, HEX);
//  chunked.print(F(">"));
//  for (uint8_t i = 5; i <= 8; i++) {
//    chunked.print(F("<option value="));
//    chunked.print(i);
//    if ((((data.config.serialConfig & 0x06) >> 1) + 5) == i) chunked.print((" selected"));
//    chunked.print(F(">"));
//    chunked.print(i);
//    chunked.print(F("</option>"));
//  }
//  chunked.print(F("</select> bit"));
//  tagDivClose(chunked);
//  tagLabelDiv(chunked, F("Parity"));
//  chunked.print(F("<select name="));
//  chunked.print(POST_PARITY, HEX);
//  chunked.print(F(">"));
//  for (uint8_t i = 0; i <= 3; i++) {
//    if (i == 1) continue;  // invalid value, skip and continue for loop
//    chunked.print(F("<option value="));
//    chunked.print(i);
//    if (((data.config.serialConfig & 0x30) >> 4) == i) chunked.print((" selected"));
//    chunked.print(F(">"));
//    switch (i) {
//      case 0:
//        chunked.print(F("None"));
//        break;
//      case 2:
//        chunked.print(F("Even"));
//        break;
//      case 3:
//        chunked.print(F("Odd"));
//        break;
//      default:
//        break;
//    }
//    chunked.print(("</option>"));
//  }
//  chunked.print(("</select>"));
//  tagDivClose(chunked);
//  tagLabelDiv(chunked, ("Stop Bits"));
//  chunked.print(("<select name="));
//  chunked.print(POST_STOP, HEX);
//  chunked.print((">"));
//  for (uint8_t i = 1; i <= 2; i++) {
//    chunked.print(("<option value="));
//    chunked.print(i);
//    if ((((data.config.serialConfig & 0x08) >> 3) + 1) == i) chunked.print((" selected"));
//    chunked.print(F(">"));
//    chunked.print(i);
//    chunked.print(F("</option>"));
//  }
//  chunked.print(F("</select> bit"));
//  tagDivClose(chunked);
//  tagLabelDiv(chunked, F("Inter-frame Delay"));
//  tagInputNumber(chunked, POST_FRAMEDELAY, byte(frameDelay() / 1000UL) + 1, 250, data.config.frameDelay, ("ms"));
//  tagDivClose(chunked);
//  tagLabelDiv(chunked, ("Response Timeout"));
//  tagInputNumber(chunked, POST_TIMEOUT, 50, 5000, data.config.serialTimeout, ("ms"));
//  tagDivClose(chunked);
//  tagLabelDiv(chunked, ("Attempts"));
//  tagInputNumber(chunked, POST_ATTEMPTS, 1, 5, data.config.serialAttempts, (""));
//  tagDivClose(chunked);
}

/**************************************************************************/
/*!
  @brief <input>
  HEX string (2 chars)

  @param chunked Chunked buffer
  @param name Name POST_
  @param required True if input is required
  @param printVal True if value is shown
  @param value Value
*/
/**************************************************************************/
void tagInputHex(ChunkedPrint &chunked, const uint8_t name, const bool required, const bool printVal, const uint8_t value) {
  chunked.print(("<input name="));
  chunked.print(name, HEX);
  if (required) {
    chunked.print((" required"));
  }
  chunked.print((" minlength=2 maxlength=2 class=i pattern='[a-fA-F&bsol;d]+' value='"));
  if (printVal) {
    chunked.print(hex(value));
  }
  chunked.print(("'>"));
}

/**************************************************************************/
/*!
  @brief <span>

  @param chunked Chunked buffer
  @param JSONKEY JSON_ id
*/
/**************************************************************************/
void tagSpan(ChunkedPrint &chunked, const uint8_t JSONKEY) {
  chunked.print(("<span id="));
  chunked.print(JSONKEY);
  chunked.print((">"));
  jsonVal(chunked, JSONKEY);
  chunked.print(("</span>"));
}

/**************************************************************************/
/*!
  @brief <input>
  IP address (4 elements)

  @param chunked Chunked buffer
  @param name Name POST_
  @param ip IP address from data.config
*/
/**************************************************************************/
void tagInputIp(ChunkedPrint &chunked, const uint8_t name, uint8_t ip[]) {
  for (uint8_t i = 0; i < 4; i++) {
    chunked.print(("<input name="));
    chunked.print(name + i, HEX);
    chunked.print((" class='p i' required maxlength=3 pattern='^(&bsol;d{1,2}|1&bsol;d&bsol;d|2[0-4]&bsol;d|25[0-5])$' value="));
    chunked.print(ip[i]);
    chunked.print((">"));
    if (i < 3) chunked.print(("."));
  }
}

/**************************************************************************/
/*!
  @brief <input type=number>

  @param chunked Chunked buffer
  @param name Name POST_
  @param min Minimum value
  @param max Maximum value
  @param value Current value
  @param units Units (string)
*/
/**************************************************************************/
void tagInputNumber(ChunkedPrint &chunked, const uint8_t name, const uint8_t min, uint16_t max, uint16_t value, const char *units) {
  chunked.print(("<input class='s n' required type=number name="));
  chunked.print(name, HEX);
  chunked.print((" min="));
  chunked.print(min);
  chunked.print((" max="));
  chunked.print(max);
  chunked.print((" value="));
  chunked.print(value);
  chunked.print(("> ("));
  chunked.print(min);
  chunked.print(("~"));
  chunked.print(max);
  chunked.print((") "));
  chunked.print(units);
}
