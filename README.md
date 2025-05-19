# Etherbridge_custom

![alt text](HW-Doc/PCB_Preview.png "Etherbridge PCB")

This repository contains the code for the custom Etherbride-board. This board is base on a STM32H533CET6 MCU in a 48-pin LQFP package. 

Attached to it is a W5500 ethernet chip from Wiznet and a 93LC86 EEPROM. The external clock is provided by a 25MHz crystal oscillator. The W5500 gets its 25 MHz clock via the MCO1-pin from the MCU.

External interfaces are CAN, RS485, an UART as extra serial interface and the SWD port for programming and debugging. Also, a LED is connected to IO-pin PH1.

## Schematic

![alt text](HW-Doc/Etherbridge_PCB-MCU.svg "MCU Schematic")

You can find the [full schematic here.](HW-Doc/Etherbridge_PCB.pdf)

## Pinout

| Pin Name  | Used As | Comment  |
| --------  | ------- | -------- |
| PH0       | HSE_IN (25 MHz) | Use HSE Bypass |
||||
| PA1       | USART2_DE (AF7) | RS485 Driver Enable |
| PA2       | USART2_TX (AF7) | RS485 TX |
| PA3       | USART2_RX (AF7) | RS485 RX |
||||
| PA4       | WS_RSTn | W5500 Reset (active low) |
| PB12      | WS_INTn | W5500 Interupt line (active low) |
| PB10      | WS_SPI_CSn | Chip select for W5500 (active low) |
| PA5       | WS_SPI1_SCK (AF5) | SPI Clock for W5500 |
| PA6       | WS_SPI1_MISO (AF5) | SPI Data In from W5500 |
| PA7       | WS_SPI1_MOSI (AF5) | SPI Data Out to W5500 |
| PA8       | WS_CLK (AF0) (MCO1 - 25MHz) | Main Clock for W5500 |
||||
|PB0        | SPI3_MISO (AF5) | SPI Data In from EEPROM |
|PB1        | SPI3_CLK (AF4)  | SPI Clock for EEPROM |
|PB2        | SPI3_MOSI       | SPI Data Out to EEPROM |
|PB13       | EEP_ORG         | EEPROM Memory Organisation: low: 8 Bit; high: 16 Bit |
|PB14       | EEP_PE          | EEPROM Programming Enable |
|PB15       | EEP_CS          | EEPROM Chip Select for SPI |
||||
| PA9       | USART1_TX (AF7) | Serial AUX TX |
| PA10      | USART1_RX (AF7) | Serial AUX RX |
||||
| PA11      | CAN1_RX (AF9) | CAN RX |
| PA12      | CAN1_TX (AF9) | CAN TX |
||||
| PA13      | SWDIO (AF0)   | SWD Data |
| PA14      | SWCLK (AF0)   | SWD Clock |
| PB3       | SWO (AF0)     | SWD Serial data out |
||||
| PH1       | GPIO_OUT | User LED - also on spare pin header|

PC13 - PC15, PA0, PA15 and PB4 - PB8 are currently not in use and connected to the spare pin header.




