/** 
* @file   Timer.h
* @brief  Software-Timer. Ben�tigt Systick.
*  
* @author Fki
*
* @version 0.1 - 13.01.2021
*/

/*******************************************************************************
*     INCLUDED FILES
*******************************************************************************/

#include "SysTick.h"
#include "Timer.h"

bool Timer::isOver() {
  if ((SysTick_GetMillis() - timestampLastHitMs) > sleepTimeMs) {
    return true;
  }
  return false;
}
void Timer::sleep(uint32_t sleepTimeMs) {
  this->sleepTimeMs = sleepTimeMs;
  timestampLastHitMs = SysTick_GetMillis();
}

void Timer::delay(uint32_t sleepTime)
{
    sleep(sleepTime);
    while(!isOver());
}
/*******************************************************************************
*     END OF FILE
*******************************************************************************/
