/*
 * Board.cpp
 *
 *  Created on: May 15, 2025
 *      Author: Fki
 */
#include "stm32h5xx.h"

#include "Board.hpp"
#include "SystemClocks.hpp"
#include "BoardPins.hpp"
#include "BoardConfig.hpp"
#include "BitsAndFields.hpp"
#include "SysTick.h"
#include "FastIo.hpp"

uint32_t SYS_uid[3];

static void updateUid(void)
{
    SYS_uid[0] = *(volatile const uint32_t*) (UID_BASE);
    SYS_uid[1] = *(volatile const uint32_t*) (UID_BASE + 4);
    SYS_uid[2] = *(volatile const uint32_t*) (UID_BASE + 8);
}

static void enableAhbClocks(void)
{
    //Enable GPIO A-D clocks and reset them.
    RCC->AHB2ENR    |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOHEN;
    RCC->AHB2RSTR   |= RCC_AHB2RSTR_GPIOARST | RCC_AHB2RSTR_GPIOBRST | RCC_AHB2RSTR_GPIOCRST | RCC_AHB2RSTR_GPIOHRST;
    RCC->AHB2RSTR   &= ~(RCC_AHB2RSTR_GPIOARST | RCC_AHB2RSTR_GPIOBRST | RCC_AHB2RSTR_GPIOCRST | RCC_AHB2RSTR_GPIOHRST);

    RCC->AHB1ENR    |= RCC_AHB1ENR_CRCEN;
    RCC->AHB1RSTR   |= RCC_AHB1RSTR_CRCRST;
    RCC->AHB1RSTR   &= ~(RCC_AHB1RSTR_CRCRST);
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

static void initTim2ForMicroSecs(TIM_TypeDef * const microTim)
{
    //Currently, only TIM2 is supported!

    if(microTim == TIM2)
    {
        // Enable TIM2 clock (Bus APB1)
        RCC->APB1LENR |= RCC_APB1LENR_TIM2EN;

        // Reset TIM2
        RCC->APB1LRSTR |= RCC_APB1LRSTR_TIM2RST;
        RCC->APB1LRSTR &= ~RCC_APB1LRSTR_TIM2RST;

        // TIM2 runs at 2 * APB1 = 500 MHz → prescaler = 499 → 1 MHz (1 µs)
        microTim->PSC = 499;
        microTim->ARR = 0xFFFFFFFF; // max 32-bit free running
        microTim->CNT = 0;

        microTim->CR1 = TIM_CR1_CEN; // Enable timer
    }
}

void BRD_init(void)
{
    SysClk_setup250MHz();
    enableMCO1Clk();
    enableAhbClocks();
    enableApb1Clocks();
    enableApb2Clocks();

    updateUid();
    McuICacheEnable();

    SysTick_Init();

    wzChannel.init(&WzIfConfig::spiConfig);
    eepIf.init(&EepIfConfig::spiConfig);
    modbus.init(ModbusConfig::baudrate);

    boardPins.init(); //Call after all other internal peripherials have been initialised.

    initTim2ForMicroSecs(microTim);

    NVIC_SetPriority(SPI1_IRQn, 0);
    NVIC_EnableIRQ(SPI1_IRQn);

    NVIC_SetPriority(SPI3_IRQn, 0);
    NVIC_EnableIRQ(SPI3_IRQn);

    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);
}



SPI_Master wzSPI(WzIfConfig::spi);
SPI_Channel wzChannel(&wzSPI, WzIfConfig::csPin, SPI_Channel::CS_Polarity::activeLow);
FastIo wzRstPin(BoardPins::Pin::W_RST_N);

SPI_Master eepSpi(EepIfConfig::spi);
SPI_Channel eepIf(&eepSpi, EepIfConfig::csPin, SPI_Channel::CS_Polarity::activeHigh);

RS485<256> modbus(ModbusConfig::usart);

extern "C" void SPI1_IRQHandler(void)
{
    wzSPI.handler();
}

extern "C" void SPI3_IRQHandler(void)
{
    eepSpi.handler();
}

extern "C" void USART2_IRQHandler(void)
{
    modbus.handler();
}
