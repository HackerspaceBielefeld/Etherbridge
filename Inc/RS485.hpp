/*
 * RS485.hpp
 *
 *  Created on: May 30, 2025
 *      Author: Fki
 */

#ifndef RS485_HPP_
#define RS485_HPP_

#include <cstdint>
#include "stm32h5xx.h"
#include "BitsAndFields.hpp"

template <size_t RX_BUF_SIZE>
class RS485
{
public:

    enum Parity
    {
        N,
        E,
        O
    };

    enum StopBits
    {
        ONE,
        TWO
    };

    explicit RS485(USART_TypeDef * const usart)
    : usart(usart)
    {}

    /**
     * General configuration:
     * Transmitter and receiver enabled
     * Parity and stopbits set from parameters, bitwidth adjusted accordingly
     * FIFOs enabled: TX threshold: empty; RX threshold: full
     * DE: active high
     */
    void init(uint32_t baudrate, Parity p = N, StopBits s = ONE)
    {
        usart->CR1 &= ~USART_CR1_UE; //Disable USART - makes parameters configurable

        uint32_t tmpCr1 = 0;
        uint32_t tmpCr2 = 0;
        uint32_t tmpCr3 = 0;

        usart->BRR = SystemCoreClock / baudrate;            //Set baudrate

        //Configure CR3: DE and FIFO thresholds
        tmpCr3 = BitsAndFields::writeBits(tmpCr3, 5, 3, USART_CR3_TXFTCFG_Pos); //Tx FIFO threshold: FIFO empty
        tmpCr3 = BitsAndFields::writeBits(tmpCr3, 5, 3, USART_CR3_RXFTCFG_Pos); //Rx FIFO threshold: FIFO full
        tmpCr3 = BitsAndFields::setBit(tmpCr3, USART_CR3_DEM_Pos);              //Activate Driver Enable signal
        tmpCr3 = BitsAndFields::setBit(tmpCr3, USART_CR3_RXFTIE_Pos);           //Enable Rx FIFO threshold interrupt
        usart->CR3 = tmpCr3;

        //Configure CR2: Stopbits
        if(s == TWO)
        {
            tmpCr2 |= USART_CR2_STOP_1;                      //Enable 2 stopbits, if selected
        }
        tmpCr2 |= USART_CR2_SWAP;
        usart->CR2 = tmpCr2;

        //Configure CR1: Enable RX, TX, FIFO mode, set data bit length and parity
        tmpCr1 = BitsAndFields::setBit(tmpCr1, USART_CR1_FIFOEN_Pos);

        if(p != N)
        {
            tmpCr1 = BitsAndFields::setBit(tmpCr1, USART_CR1_M0_Pos);   //Add parity bit
            tmpCr1 = BitsAndFields::setBit(tmpCr1, USART_CR1_PCE_Pos);  //Ennable parity controll

            if(p == O)
            {
                tmpCr1 = BitsAndFields::setBit(tmpCr1, USART_CR1_PS_Pos); //Parity = Odd
            }
        }

        tmpCr1 = BitsAndFields::writeBits(tmpCr1, 1, 5, USART_CR1_DEAT_Pos); //Driver assertion time = 1/16 bit time (1 sample time)
        tmpCr1 = BitsAndFields::writeBits(tmpCr1, 1, 5, USART_CR1_DEDT_Pos); //Driver deassertion time = 1/16 bit time (1 sample time)
        tmpCr1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_IDLEIE | USART_CR1_UE;  //Enable Tx, RX, IDLE interrupt and USART peripherial in general
        usart->CR1 = tmpCr1;
    }

    ErrorStatus send(uint8_t * data, size_t len)
    {
        if(isBusy() || len == 0)
        {
            return ERROR;
        }

        txBuf = data;
        txBufSize = len;
        txBufCnt = 0;

        usart->CR3 |= USART_CR3_TXFTIE;
        return SUCCESS;
    }

    size_t isRxAvail(void) const
    {
        if(rxDone)
        {
            return rxBufCnt;
        }
        return 0;
    }

    bool isRxDone(void) const
    {
        return rxDone;
    }

    uint8_t * getRxBuffer(void)
    {
        return rxBuf;
    }

    ErrorStatus freeRxBuffer(void)
    {
        if(isBusy())
        {
            return ERROR;
        }

        rxBufCnt = 0;
        rxError = false;
        rxDone = false;

        return SUCCESS;
    }

    bool isBusy() const
    {
        uint32_t sr = usart->ISR;

        if(sr & (USART_ISR_BUSY) || ((sr & USART_ISR_TC) == 0))
        {
            return true;
        }
        return false;
    }

    void handler(void)
    {
        //Handle RX:
        while(usart->ISR & USART_ISR_RXNE)
        {
            if(rxBufCnt < RX_BUF_SIZE)
            {
                rxBuf[rxBufCnt++] = usart->RDR;
            }
            else
            {
                uint8_t dummy = usart->RDR;
                rxError = true;
            }

            if(usart->ISR & (USART_ISR_PE | USART_ISR_ORE))
            {
                rxError = true;
                usart->ICR = (USART_ICR_PECF | USART_ICR_ORECF);
            }
        }

        if(usart->ISR & USART_ISR_IDLE)
        {
            rxDone = true;
            usart->ICR = USART_ICR_IDLECF;
        }

        //Handle TX
        while((txBufCnt < txBufSize) && (usart->ISR & USART_ISR_TXE_TXFNF))
        {
            usart->TDR = txBuf[txBufCnt++];
            if(txBufCnt == txBufSize)
            {
                usart->CR3 &= ~(USART_CR3_TXFTIE);
            }
        }


    }
private:

    USART_TypeDef * const usart;

    uint8_t rxBuf[RX_BUF_SIZE];

    volatile size_t rxBufCnt = 0;
    volatile bool rxError = false;
    volatile bool rxDone = false;

    uint8_t * txBuf = nullptr;
    size_t txBufSize = 0;
    volatile size_t txBufCnt = 0;
};


#endif /* RS485_HPP_ */
