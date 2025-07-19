/*
 * FastIo.h
 *
 *  Created on: May 22, 2025
 *      Author: Fki
 */

#ifndef FASTIO_HPP_
#define FASTIO_HPP_

#include <cstdint>
#include "stm32h5xx.h"
#include "BitsAndFields.hpp"
#include "BoardPins.hpp"

class FastIo
{
private:
    GPIO_TypeDef *const port;
    const uint32_t mask;

public:
    constexpr FastIo(BoardPins::Pin boardPin)
    : port(boardPins.getPort(boardPin)),
      mask(BitsAndFields::setBitMask(1, boardPins.getPinPos(boardPin)))
    {}

    void set() const
    {
        port->BSRR |= mask;
    }

    void clr() const
    {
        port->BRR |= mask;
    }

    bool get() const
    {
        return port->IDR & mask;
    }
};



#endif /* FASTIO_HPP_ */
