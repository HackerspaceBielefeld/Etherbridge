/*
 * MicroTimer.cpp
 *
 *  Created on: Jun 4, 2025
 *      Author: Fki
 */

// micro_timer.cpp
#include "MicroTimer.hpp"

MicroTimer::MicroTimer(TIM_TypeDef* timer)
    : _tim(timer), _start(0), _duration(0) { }

void MicroTimer::sleep(uint32_t sleepTime)
{
    _start = _tim->CNT;
    _duration = sleepTime;
}

bool MicroTimer::isOver() {
    uint32_t now = _tim->CNT;
    return (now - _start) >= _duration;
}

uint32_t MicroTimer::micros(void)
{
    return _tim->CNT;
}

