/*
 * BoardConfig.hpp
 *
 *  Created on: May 21, 2025
 *      Author: Fki
 */

#ifndef BOARDCONFIG_HPP_
#define BOARDCONFIG_HPP_

#include <cstdint>
#include "BitsAndFields.hpp"
#include "stm32h5xx.h"
#include "SPI_Master.h"
#include "BoardPins.hpp"

/*
 * Flash configuration parameters
 *
 * Depends on selected clock speed!
 */
namespace FlashConfig
{
constexpr uint32_t flashLatency = FLASH_ACR_LATENCY_5WS;
constexpr uint32_t flashWrFreq = FLASH_ACR_WRHIGHFREQ_1;
constexpr uint32_t flasPrefetch = FLASH_ACR_PRFTEN;
}

namespace ICacheConfig
{
constexpr uint32_t mismon = ICACHE_CR_MISSMEN;
constexpr uint32_t hitmon = ICACHE_CR_HITMEN;
constexpr uint32_t enable = ICACHE_CR_EN;
}

namespace HseConfig
{
constexpr uint32_t value = 25000000;         //25MHz
constexpr uint32_t bypass = RCC_CR_HSEBYP;   //External clock source, NOT Crystal!
constexpr uint32_t extClkType = 0;           //0 = analog mode, 1 = digital mode
}

//Configuration for PLL 1. See Reference Manual for further details.
namespace PllConfig
{
//For details regarding pll configuration, see RefMan, p.460
constexpr uint8_t preDivFactor = 20;    //25MHz / 20 = 1,25MHz
constexpr uint8_t postDivFactor = 1;    //Output diviter = 1
constexpr uint16_t multFactor = 200;    //VCO multiplier = 200; 200 + 1,25MHz = 250MHz
constexpr uint8_t clkSrcHSE = 3;        //Clock Source = HSE

constexpr uint8_t preDivFieldWidth = 6;
constexpr uint8_t divFieldWidth = 7;
constexpr uint8_t multFieldWidth = 9;
constexpr uint8_t clkSrcFieldWidth = 2;

//No output division
constexpr uint32_t P_OutputDivider = BitsAndFields::writeBits(0, postDivFactor-1, divFieldWidth, RCC_PLL1DIVR_PLL1P_Pos);
constexpr uint32_t Q_OutputDivider = BitsAndFields::writeBits(0, postDivFactor-1, divFieldWidth, RCC_PLL1DIVR_PLL1Q_Pos);
constexpr uint32_t R_OutputDivider = BitsAndFields::writeBits(0, postDivFactor-1, divFieldWidth, RCC_PLL1DIVR_PLL1R_Pos);
//VCO multiplier = 200 * 1,25MHz = 250MHz
constexpr uint32_t vcoMulti = BitsAndFields::writeBits(0, multFactor-1, multFieldWidth, RCC_PLL1DIVR_PLL1N_Pos);
//Only Output P is enabled
constexpr uint32_t P_OutputEnable = RCC_PLL1CFGR_PLL1PEN;
constexpr uint32_t Q_OutputEnable = RCC_PLL1CFGR_PLL1QEN; //SPI kernel clock
constexpr uint32_t R_OutputEnable = 0;
constexpr uint32_t prescaler = BitsAndFields::writeBits(0, preDivFactor, preDivFieldWidth, RCC_PLL1CFGR_PLL1M_Pos); //(1,25MHz Input Clk)
constexpr uint32_t vcoRangeSelector = RCC_PLL1CFGR_PLL1VCOSEL_Pos; //medium VCO range (150 - 420MHz)
constexpr uint32_t inputClkRange = 0; //1 - 2 MHz
constexpr uint32_t pllSrc = BitsAndFields::writeBits(0, clkSrcHSE, clkSrcFieldWidth, RCC_PLL1CFGR_PLL1SRC_Pos);
}

namespace MCO1Config
{
constexpr uint8_t prescaler = 1;
constexpr uint8_t prescalerFieldWidth = 4;

constexpr uint8_t clockSrc = 2; // 0: HSI; 1: LSE; 2: HSE; 3: PLL1_Q; 4 HSI48
constexpr uint8_t clockSrcFieldWidth = 3;
}

namespace WzIfConfig
{
constexpr BoardPins::Pin csPin = BoardPins::Pin::WZ_CS_N;
SPI_TypeDef * const spi = SPI1;
constexpr SPI_Master::SPI_Config spiConfig = {
    SPI_Master::Prescaler::DIV_8,
    SPI_Master::SPI_Mode::MODE_0,
    SPI_Master::BitOrder::MSB_FIRST
};
}

namespace EepIfConfig
{
constexpr BoardPins::Pin csPin = BoardPins::Pin::EEP_CS;
SPI_TypeDef * const spi = SPI3;
constexpr SPI_Master::SPI_Config spiConfig = {
    SPI_Master::Prescaler::DIV_256,
    SPI_Master::SPI_Mode::MODE_0,
    SPI_Master::BitOrder::MSB_FIRST
};
}

namespace ModbusConfig
{
USART_TypeDef * const usart = USART2;
constexpr uint32_t baudrate = 115200;
constexpr uint8_t bitsPerByte = 10; //8N1: 1 Startbit + 8 Databist + 1 Stopbit
}

TIM_TypeDef * const microTim = TIM2;


#endif /* BOARDCONFIG_HPP_ */
