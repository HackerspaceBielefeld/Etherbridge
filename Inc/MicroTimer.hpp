/*
 * MicroTimer.hpp
 *
 *  Created on: Jun 4, 2025
 *      Author: Fki
 */

#ifndef MICROTIMER_HPP_
#define MICROTIMER_HPP_

#include <stdint.h>
#include "stm32h5xx.h"

class MicroTimer {
public:
    MicroTimer(TIM_TypeDef* timer);
    void sleep(uint32_t sleepTimeMs);
    bool isOver();
    uint32_t micros();

private:
    TIM_TypeDef* _tim;
    uint32_t _start;
    uint32_t _duration;
};




#endif /* MICROTIMER_HPP_ */
