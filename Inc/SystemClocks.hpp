/**
* @file     SystemClocks.hpp
* @brief    Functions for clock setup to 250MHz and enabling ICache.
* @see      SystemClocks.cpp
*
* @author   Fki
* @date     11.03.2025
*
* @version  1.0 - Initial version.
*/
#ifndef SYSTEMCLOCKS_HPP_
#define SYSTEMCLOCKS_HPP_

void SysClk_setup250MHz(void);
void McuICacheEnable(void);

#endif /* SYSTEMCLOCKS_HPP_ */
