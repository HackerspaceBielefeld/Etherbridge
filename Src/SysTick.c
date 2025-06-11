/** 
* @file   SysTick.h
* @brief  Konfiguriert den Systick und aktiviert dessen Interrupt.
* @see    SysTick.c
*  
* @author Fki
* @date   22.02.2018
*
* @version 0.1 
* Initiale Version.
*/

/*******************************************************************************
*     INCLUDED FILES
*******************************************************************************/

//Hardware dependent libraries
#if defined(STM32F0)
  #include "stm32f0xx.h"
  
#elif defined(STM32L4)
  #include "stm32l4xx.h"

#elif defined(STM32F4)
#include "stm32f4xx.h"

#elif defined(STM32H5)
#include "stm32h5xx.h"
#endif

#include "SysTick.h"

/*******************************************************************************
*     DEFINITIONS
*******************************************************************************/

#ifndef SysTick_IRQ_PRIO
#define SysTick_IRQ_PRIO 0
#endif

/*******************************************************************************
*     PRIVATE TYPES
*******************************************************************************/
 
/*******************************************************************************
*     PRIVATE DATA
*******************************************************************************/

static volatile uint32_t SysTickCnt;

/*******************************************************************************
*     PUBLIC DATA
*******************************************************************************/

/**
* @brief Dieses Flag wird im 1ms-Takt vom Interrupt gesetzt.
*/
volatile bool SysTick_msTick;

/*******************************************************************************
*     PRIVATE FUNCTIONS
*******************************************************************************/

/*******************************************************************************
*     PUBLIC FUNCTIONS
*******************************************************************************/

/**
* @brief Initialisiert den Systick-Counter so, dass alle 1ms das Tick-Flag gesetzt wird.

* @return void
*/
void SysTick_Init(void)
{
  SysTick_msTick = false;
  SysTickCnt = 0;
  
  SysTick->LOAD = ((SystemCoreClock / 8) / 1000) - 1;
  SysTick->CTRL &= ~(SysTick_CTRL_CLKSOURCE_Msk); //prescaler active


  SysTick->VAL   = 0UL;
  SysTick->CTRL |= (SysTick_CTRL_TICKINT_Msk
                |   SysTick_CTRL_ENABLE_Msk);         /* Enable SysTick IRQ and SysTick Timer */

  NVIC_SetPriority(SysTick_IRQn, SysTick_IRQ_PRIO);
#if defined(STM32F0)
  NVIC_EnableIRQ(SysTick_IRQn);
#endif
}


/**
* @brief SysTick-Interrupt, bedient das msTick-Flag.

* @return void
*/
void SysTick_Handler(void)
{
  SysTickCnt++;
  SysTick_msTick = true;
}

uint32_t SysTick_GetMillis(void)
{
  return SysTickCnt;
}

uint16_t SysTick_GetRandom(void)
{
    return SysTick->VAL & 0xffff;
}

/*******************************************************************************
*     END OF FILE
*******************************************************************************/
