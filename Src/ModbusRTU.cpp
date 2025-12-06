/*
 * ModbusRTU.cpp
 *
 *  Created on: Jun 6, 2025
 *      Author: Fki
 */

#include "ModbusRTU.h"
#include "main.h"
#include "ModbusTCP.h"
#include "Board.hpp"
#include "ProjectSettings.h"
#include "crc.hpp"
#include "RS485.hpp"
#include "BoardConfig.hpp"

enum class SerialState : uint8_t
{
  IDLE,
  SENDING,
  DELAY,
  WAITING
};

static uint8_t txBuffer[MODBUS_SIZE];
SerialState serialState;

/* *******************************************************************
   Modbus RTU functions

   sendSerial()
   - sends Modbus RTU requests to HW serial port (RS485 interface)

   recvSerial()
   - receives Modbus RTU replies
   - adjusts headers and forward messages as Modbus TCP/UDP or Modbus RTU over TCP/UDP
   - sends Modbus TCP/UDP error messages in case Modbus RTU response timeouts

   checkCRC()
   - checks an array and returns true if CRC is OK

   calculateCRC()

   ***************************************************************** */

void sendSerial()
{
	if (!sendMicroTimer.isOver())
	{
		return;
	}
	if (queueHeaders.isEmpty())
	{
		return;
	}

	static uint8_t txNdx = 0;

	header_t myHeader = queueHeaders.first();

	switch (serialState)
	{
    case SerialState::IDLE:  //Optimize queue (prioritize requests from responding slaves) and trigger sending via serial
    	while (priorityReqInQueue && (queueHeaders.first().requestType & PRIORITY_REQUEST) == false)
    	{
    		// move requests to non responding slaves to the tail of the queue
    		for (uint8_t i = 0; i < queueHeaders.first().msgLen; i++)
    		{
    			queueData.push(queueData.shift());
    		}
    		queueHeaders.push(queueHeaders.shift());
    	}
    	serialState = SerialState::SENDING;
    	break;

    case SerialState::SENDING:
		{
			if (txNdx == 0)
			{
				crc = 0xFFFF;
			}
			while (txNdx < myHeader.msgLen)
			{
				txBuffer[txNdx] = queueData[txNdx];
				calculateCRC(queueData[txNdx]);
				txNdx++;
			}
			if (txNdx == myHeader.msgLen)
			{
				txBuffer[txNdx++] = lowByte(crc);  // send CRC, low byte first
				txBuffer[txNdx++] = highByte(crc);
			}
			if (txNdx > myHeader.msgLen)
			{
				// wait for last byte (incl. CRC) to be sent from serial Tx buffer
				// this if statement is not very reliable (too fast)
				// Serial.isFlushed() method is needed....see https://github.com/arduino/Arduino/pull/3737
				if(modbus.send(txBuffer, txNdx) == SUCCESS)
				{
					txNdx = 0;
					serialState = SerialState::DELAY;
				}
			}
		}
		break;

    case SerialState::DELAY:
		{
			#ifdef ENABLE_EXTENDED_WEBUI
			data.rtuCnt[DATA_TX] += myHeader.msgLen;
			data.rtuCnt[DATA_TX] += 2;
			#endif
			myHeader.atts++;
			queueHeaders.shift();
			queueHeaders.unshift(myHeader);
			uint32_t delay = data.config.serialTimeout;
			if (myHeader.requestType & SCAN_REQUEST)
			{
				delay = SCAN_TIMEOUT;  // fixed timeout for scan requests
			}
			sendMicroTimer.sleep(delay * 1000UL);
			serialState = SerialState::WAITING;
		}
		break;

    case SerialState::WAITING: //Deal with Serial timeouts (i.e. Modbus RTU timeouts)
		{
			if (myHeader.requestType & SCAN_REQUEST)
			{  // Only one attempt for scan request (we do not count attempts)
				deleteRequest();
			}
			else if (myHeader.atts >= data.config.serialAttempts)
			{
				// send modbus error 0x0B (Gateway Target Device Failed to Respond) - usually means that target device (address) is not present
				setSlaveStatus(queueData[0], SLAVE_ERROR_0B, true, false);
				uint8_t MBAP[] =
				{
				    myHeader.tid[0],
					myHeader.tid[1],
					0x00,
					0x00,
					0x00,
					0x03
				};
				uint8_t PDU[5] =
				{
    			    queueData[0],
					uint8_t(queueData[1] + 0x80),
					0x0B
				};
				crc = 0xFFFF;
				for (uint8_t i = 0; i < 3; i++)
				{
					calculateCRC(PDU[i]);
				}
				PDU[3] = lowByte(crc);  // send CRC, low byte first
				PDU[4] = highByte(crc);
				sendResponse(MBAP, PDU, 5);
				data.errorCnt[ERROR_TIMEOUT]++;
			}
			else
			{
				setSlaveStatus(queueData[0], SLAVE_ERROR_0B_QUEUE, true, false);
				data.errorCnt[ERROR_TIMEOUT]++;
			}                 // if (myHeader.atts >= MAX_RETRY)
			serialState = SerialState::IDLE;
		}
		break;
    default:
    	break;
	}
}

void recvSerial()
{
	static uint16_t rxNdx = 0;
	static uint8_t * serialIn;

	rxNdx = modbus.isRxAvail();

	if(rxNdx > 0)
	{
		sendMicroTimer.sleep(data.config.frameDelay * 1000UL);  // delay next serial write

		// Process Serial data
		// Checks: 1) CRC; 2) address of incoming packet against first request in queue; 3) only expected responses are forwarded to TCP/UDP
		header_t myHeader = queueHeaders.first();
		serialIn = modbus.getRxBuffer();

		if (checkCRC(serialIn, rxNdx) == true && serialIn[0] == queueData[0] && serialState == SerialState::WAITING)
		{
			if (serialIn[1] > 0x80 && (myHeader.requestType & SCAN_REQUEST) == false)
			{
				setSlaveStatus(serialIn[0], SLAVE_ERROR_0X, true, false);
			}
			else
			{
				setSlaveStatus(serialIn[0], SLAVE_OK, true, myHeader.requestType & SCAN_REQUEST);
			}
			uint8_t MBAP[] =
			{
                myHeader.tid[0],
				myHeader.tid[1],
				0x00,
				0x00,
				highByte(rxNdx - 2),
				lowByte(rxNdx - 2)
			};

			sendResponse(MBAP, serialIn, rxNdx);
			serialState = SerialState::IDLE;
		}
		else
		{
			data.errorCnt[ERROR_RTU]++;
		}

		#ifdef ENABLE_EXTENDED_WEBUI
		data.rtuCnt[DATA_RX] += rxNdx;
		#endif /* ENABLE_EXTENDED_WEBUI */

		rxNdx = 0;
		while(modbus.freeRxBuffer() != SUCCESS);
	}
}

uint32_t charTimeOut()
{
	if (ModbusConfig::baudrate <= 19200)
	{
		return (1500000UL * 10) / ModbusConfig::baudrate;  // inter-character time-out should be 1,5T
	}
	else
	{
		return 750;
	}
}
