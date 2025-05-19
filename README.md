# Etherbridge_custom

This repository contains the code for the custom Etherbride-board. This board is base on a STM32H533CET6 MCU in a 48-pin LQFP package. 

Attached to it is a W5500 ethernet chip from Wiznet and a 93LC86 EEPROM. The external clock is provided by a 25MHz crystal oscillator. The W5500 gets its 25 MHz clock via the MCO1-pin from the MCU.

External interfaces are CAN, RS485, an UART as local interface and the SWD port for programming and debugging. Also, a LED is connected to one IO-pin.

## Schematic

![alt text](HW-Doc/Etherbridge_PCB-MCU.svg "MCU Schematic")

You can find the full schematic here:


