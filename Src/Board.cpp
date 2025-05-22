/*
 * Board.cpp
 *
 *  Created on: May 15, 2025
 *      Author: Fki
 */
#include "stm32h5xx.h"

#include "SystemClocks.hpp"
#include "BoardPins.hpp"
#include "BoardConfig.hpp"
#include "BitsAndFields.hpp"
#include "SysTick.h"
#include "FastIo.hpp"

static void enableGpioClocks(void)
{
    //Enable GPIO A-D clocks and reset them.
    RCC->AHB2ENR    |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOHEN;
    RCC->AHB2RSTR   |= RCC_AHB2RSTR_GPIOARST | RCC_AHB2RSTR_GPIOBRST | RCC_AHB2RSTR_GPIOCRST | RCC_AHB2RSTR_GPIOHRST;
    RCC->AHB2RSTR   &= ~(RCC_AHB2RSTR_GPIOARST | RCC_AHB2RSTR_GPIOBRST | RCC_AHB2RSTR_GPIOCRST | RCC_AHB2RSTR_GPIOHRST);

    //Enable USART2 + SPI3 clocks and reset them
    RCC->APB1LENR   |= RCC_APB1LENR_SPI3EN | RCC_APB1LENR_USART2EN;
    RCC->APB1LRSTR  |= RCC_APB1LRSTR_SPI3RST | RCC_APB1LRSTR_USART2RST;
    RCC->APB1LRSTR  &= ~(RCC_APB1LRSTR_SPI3RST | RCC_APB1LRSTR_USART2RST);

    //Enable USART1 + SPI1 clocks and reset them.
    RCC->APB2ENR   |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USART1EN;
    RCC->APB2RSTR  |= RCC_APB2RSTR_SPI1RST | RCC_APB2RSTR_USART1RST;
    RCC->APB2RSTR  &= ~(RCC_APB2RSTR_SPI1RST | RCC_APB2RSTR_USART1RST);
}

static void enableApb1Clocks(void)
{
    //Enable USART2 + SPI3 clocks and reset them
    RCC->APB1LENR   |= RCC_APB1LENR_SPI3EN | RCC_APB1LENR_USART2EN;
    RCC->APB1LRSTR  |= RCC_APB1LRSTR_SPI3RST | RCC_APB1LRSTR_USART2RST;
    RCC->APB1LRSTR  &= ~(RCC_APB1LRSTR_SPI3RST | RCC_APB1LRSTR_USART2RST);

    //Enable FDCAN1+2
    RCC->APB1HENR   |= RCC_APB1HENR_FDCANEN;
    RCC->APB1HRSTR  |= RCC_APB1HRSTR_FDCANRST;
    RCC->APB1HRSTR  &= ~(RCC_APB1HRSTR_FDCANRST);
}

static void enableApb2Clocks(void)
{
    //Enable USART1 + SPI1 clocks and reset them.
    RCC->APB2ENR   |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USART1EN;
    RCC->APB2RSTR  |= RCC_APB2RSTR_SPI1RST | RCC_APB2RSTR_USART1RST;
    RCC->APB2RSTR  &= ~(RCC_APB2RSTR_SPI1RST | RCC_APB2RSTR_USART1RST);
}

static void enableMCO1Clk(void)
{
    uint32_t rccReg = RCC->CFGR1;
    rccReg = BitsAndFields::writeBits(rccReg, MCO1Config::clockSrc, MCO1Config::clockSrcFieldWidth, RCC_CFGR1_MCO1SEL_Pos);
    rccReg = BitsAndFields::writeBits(rccReg, MCO1Config::prescaler, MCO1Config::prescalerFieldWidth, RCC_CFGR1_MCO1PRE_Pos);
    RCC->CFGR1 = rccReg;
}

void BRD_init(void)
{
    SysClk_setup250MHz();
    enableMCO1Clk();
    enableGpioClocks();
    enableApb1Clocks();
    enableApb2Clocks();

    SysTick_Init();

    boardPins.init(); //Call after all other internal peripherials have been initialised.
}




