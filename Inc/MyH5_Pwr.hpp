/**
* @file     MyH5_Pwr.cpp
* @brief    Driver for power control.
*
*           Currently supports adjusting the VOSx level settings. Needed for
*           switching to higher operation frequencies.
*
* @author   Fki
* @date     16.03.2025
*
* @version  1.0 - Initial version.
*/

#ifndef MYH5_PWR_HPP
#define MYH5_PWR_HPP

/*******************************************************************************
*     INCLUDED FILES
*******************************************************************************/

#include "stm32h5xx.h"

namespace MyH5::Pwr{

enum class VoltageLevel
{
    VOS3 = 0,
    VOS2 = 1,
    VOS1 = 2,
    VOS0 = 3
};

/**
 * Returns the voltage output scaling currently applied to VCore
 */
inline VoltageLevel getCurrentVOS(void){
    return static_cast<VoltageLevel>((PWR->VOSSR & PWR_VOSSR_ACTVOS_Msk) >> PWR_VOSSR_ACTVOS_Pos);
}

/**
 * Increases the VCore by one VOS level (level number goes down). If current VOS is the highest level,
 * no changes will be done.
 *
 * BLOCKING FUNCTION
 */
static void incVoltage(void){
    VoltageLevel cVoltLevel = getCurrentVOS();

    if(cVoltLevel < VoltageLevel::VOS0){
        PWR->VOSCR = (static_cast<uint32_t>(cVoltLevel) + 1) << PWR_VOSCR_VOS_Pos;

        while((PWR->VOSSR & (PWR_VOSSR_VOSRDY | PWR_VOSSR_ACTVOSRDY)) != (PWR_VOSSR_VOSRDY | PWR_VOSSR_ACTVOSRDY));
    }
}

/**
 * Decreases the VCore by one VOS level (level number goes up). If current VOS is the lowest level,
 * no changes will be done.
 *
 * BLOCKING FUNCTION
 */
static void decVoltage(void)
{
    VoltageLevel cVoltLevel = getCurrentVOS();

    if(cVoltLevel > VoltageLevel::VOS3)
    {
        PWR->VOSCR = (static_cast<uint32_t>(cVoltLevel) - 1) << PWR_VOSCR_VOS_Pos;

        while((PWR->VOSSR & PWR_VOSSR_ACTVOSRDY) != PWR_VOSSR_ACTVOSRDY);
    }
}

/**
 * Adjusts the VCore to the selected VOS-Level.
 *
 * BLOCKING FUNCTION
 */
static void setVOS(VoltageLevel vl)
{
    while(vl != getCurrentVOS())
    {
        if(vl > getCurrentVOS())
        {
            incVoltage();
        }
        else
        {
            decVoltage();
        }
    }
}

}

#endif //MYH5_PWR_HPP
