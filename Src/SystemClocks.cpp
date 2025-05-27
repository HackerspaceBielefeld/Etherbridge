/**
* @file     SystemClocks.cpp
* @brief    Sets up the different clocks in the MCU.
*
*           Offers functions to set the system clock and bus clocks to 250MHz.
*
* @author   Fki
* @date     11.03.2025
*
* @version  1.0 - Initial version.
*/


/*******************************************************************************
*     INCLUDED FILES
*******************************************************************************/

#include "SystemClocks.hpp"
#include "MyH5_Pwr.hpp"
#include "BoardConfig.hpp"
#include "BitsAndFields.hpp"

#include "stm32h5xx.h"

/**
 * Enables the instruction cache and the hit and miss monitors
 */
static void McuICacheEnable(void)
{
    constexpr uint32_t tmpCr = ICacheConfig::mismon | ICacheConfig::hitmon | ICacheConfig::enable;
    ICACHE->CR = tmpCr;
}

/**
 * Configure flash latency, programming delay and prefetch. See RefMan, p.251
 */
static void setFlashConfig(void)
{
    constexpr uint32_t flashAcV = FlashConfig::flashLatency | FlashConfig::flashWrFreq | FlashConfig::flasPrefetch;
    constexpr uint32_t flashAcM = FLASH_ACR_LATENCY | FLASH_ACR_WRHIGHFREQ | FLASH_ACR_PRFTEN;

    while((FLASH->ACR & flashAcM) != flashAcV)
    {
        FLASH->ACR = flashAcV;
    }
}

/**
 * Configures the system and bus clock to be on 250MHz.
 *
 * The following steps are necessary:
 * Rising the core voltage level to VOS0
 * Configure the flash latency (RefMan p. 251)
 * Set the prescalers for the busses (RefMan p. 488)
 * Enable HSE oscillator
 * Configure and enable PLL (RefMan p. 496)
 * Perform clock switch
 * Update SystemCoreClock.
 */

void SysClk_setup250MHz(void)
{
    //Set voltagelevel to VOS0
    MyH5::Pwr::setVOS(MyH5::Pwr::VoltageLevel::VOS0);

    setFlashConfig();

    //Setting prescaler for AHB and APB busses (AHB1, APB1, APB2, APB3)
    //Set all prescalers to 1, since all busses are capable of handling 250MHz
    //See RefMan, p.488
    RCC->CFGR2 = 0;

    //Enable HSE
    RCC->CR |= HseConfig::bypass | HseConfig::extClkType;
    RCC->CR |= RCC_CR_HSEON;
    while(!(RCC->CR & RCC_CR_HSERDY));

    //Configure PLL
    //all output prescalers will be set to /2, multiplier will be set to 125, see RefMan, p.496
    constexpr uint32_t divReg = PllConfig::P_OutputDivider | PllConfig::Q_OutputDivider | PllConfig::R_OutputDivider
							  | PllConfig::vcoMulti;
    RCC->PLL1DIVR = divReg;

    //Enable PLL-output P, set input divider to 6, select input frequency range 4 - 8 MHz and HSE as clock source
    constexpr uint32_t pllCfgr = PllConfig::P_OutputEnable | PllConfig::Q_OutputEnable | PllConfig::R_OutputEnable
							   | PllConfig::prescaler | PllConfig::vcoRangeSelector | PllConfig::inputClkRange 
						       | PllConfig::pllSrc;
    RCC->PLL1CFGR = pllCfgr;

    //Switch on PLL1
    RCC->CR |= RCC_CR_PLL1ON;
    while(!(RCC->CR & RCC_CR_PLL1RDY));

    //Perform system clock switch
    uint32_t tmpCfgReg = RCC->CFGR1;
    tmpCfgReg = BitsAndFields::writeBitField(tmpCfgReg, 3, 2, RCC_CFGR1_SW_Pos);
    RCC->CFGR1 = tmpCfgReg;
    while(((RCC->CFGR1 & RCC_CFGR1_SWS) >> RCC_CFGR1_SWS_Pos) != 3);

    SystemCoreClockUpdate();
    McuICacheEnable();
    //Done
}
