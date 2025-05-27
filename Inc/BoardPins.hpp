/**
* Pin names and initialisation data for use with the custom Etherbridge-board.
*/

#ifndef BOARD_SETTINGS_H
#define BOARD_SETTINGS_H

#ifdef STM32H533CETx

#include <cstdint>
#include "stm32h5xx.h"
#include "BitsAndFields.hpp"

class BoardPins
{
public:

    //Warning: Values are array indices! Do not change!
    enum class Pin : uint8_t
    {
        RS485_DE    = 0,
        RS485_TX    = 1,
        RS485_RX    = 2,

        WZ_RST_N    = 3,
        WZ_INT_N    = 4,
        WZ_CS_N     = 5,
        WZ_SCK      = 6,
        WZ_MISO     = 7,
        WZ_MOSI     = 8,
        WZ_CLK      = 9,

        EEP_CS      = 10,
        EEP_SCK     = 11,
        EEP_MISO    = 12,
        EEP_MOSI    = 13,
        EEP_ORG     = 14,
        EEP_PE      = 15,

        SER_TX      = 16,
        SER_RX      = 17,

        CAN_RX      = 18,
        CAN_TX      = 19,

        SWDIO       = 20,
        SWCLK       = 21,
        SWO         = 22,

        LED         = 23,

        IO_PA0      = 24,
        IO_PA15     = 25,
        IO_PB4      = 26,
        IO_PB5      = 27,
        IO_PB6      = 28,
        IO_PB7      = 29,
        IO_PB8      = 30,
        IO_PC13     = 31,
        IO_PC14     = 32,
        IO_PC15     = 33
    };

    enum class Port : uint8_t
    {
        PORTA = 0,
        PORTB = 1,
        PORTC = 2,
        PORTH = 3
    };

    enum class Mode : uint8_t
    {
        INPUT   = 0,
        OUTPUT  = 1,
        AF      = 2,
        ANALOG  = 3
    };

    constexpr GPIO_TypeDef* getPort(Pin pin)
    {
        uint8_t pinNr = static_cast<uint8_t>(pin);
        uint8_t portNr = static_cast<uint8_t>(pinConf[pinNr].port);
        return ports[portNr];
    }

    constexpr uint8_t getPinPos(Pin pin)
    {
        uint8_t pinNr = static_cast<uint8_t>(pin);
        return pinConf[pinNr].pinPos;
    }

    void init()
    {
        //To prevent glitches, MODER should be set last per port.
        ports[static_cast<uint8_t>(Port::PORTA)]->OSPEEDR   = getReg(Port::PORTA, ConfField::SpeedReg);
        ports[static_cast<uint8_t>(Port::PORTA)]->AFR[0]    = getReg(Port::PORTA, ConfField::AflReg);
        ports[static_cast<uint8_t>(Port::PORTA)]->AFR[1]    = getReg(Port::PORTA, ConfField::AfhReg);
        ports[static_cast<uint8_t>(Port::PORTA)]->ODR       = getReg(Port::PORTA, ConfField::OdReg);
        ports[static_cast<uint8_t>(Port::PORTA)]->MODER     = getReg(Port::PORTA, ConfField::ModeReg);

        ports[static_cast<uint8_t>(Port::PORTB)]->OSPEEDR   = getReg(Port::PORTB, ConfField::SpeedReg);
        ports[static_cast<uint8_t>(Port::PORTB)]->AFR[0]    = getReg(Port::PORTB, ConfField::AflReg);
        ports[static_cast<uint8_t>(Port::PORTB)]->AFR[1]    = getReg(Port::PORTB, ConfField::AfhReg);
        ports[static_cast<uint8_t>(Port::PORTB)]->ODR       = getReg(Port::PORTB, ConfField::OdReg);
        ports[static_cast<uint8_t>(Port::PORTB)]->MODER     = getReg(Port::PORTB, ConfField::ModeReg);

        ports[static_cast<uint8_t>(Port::PORTC)]->OSPEEDR   = getReg(Port::PORTC, ConfField::SpeedReg);
        ports[static_cast<uint8_t>(Port::PORTC)]->AFR[0]    = getReg(Port::PORTC, ConfField::AflReg);
        ports[static_cast<uint8_t>(Port::PORTC)]->AFR[1]    = getReg(Port::PORTC, ConfField::AfhReg);
        ports[static_cast<uint8_t>(Port::PORTC)]->ODR       = getReg(Port::PORTC, ConfField::OdReg);
        ports[static_cast<uint8_t>(Port::PORTC)]->MODER     = getReg(Port::PORTC, ConfField::ModeReg);

        ports[static_cast<uint8_t>(Port::PORTH)]->OSPEEDR   = getReg(Port::PORTH, ConfField::SpeedReg);
        ports[static_cast<uint8_t>(Port::PORTH)]->AFR[0]    = getReg(Port::PORTH, ConfField::AflReg);
        ports[static_cast<uint8_t>(Port::PORTH)]->AFR[1]    = getReg(Port::PORTH, ConfField::AfhReg);
        ports[static_cast<uint8_t>(Port::PORTH)]->ODR       = getReg(Port::PORTH, ConfField::OdReg);
        ports[static_cast<uint8_t>(Port::PORTH)]->MODER     = getReg(Port::PORTH, ConfField::ModeReg);
    }


private:

    enum class Speed: uint8_t
    {
        LS  = 0,
        MS  = 1,
        HS  = 2,
        VHS = 3
    };

    struct PinCfg
    {
        Port    port    : 2;    //Index in port array
        uint8_t pinPos  : 4;    //Pin pos in port register
        Mode    mode    : 2;    //Pin mode
        Speed   speed   : 2;    //Pin driver strength
        uint8_t af      : 4;    //Pin alternate function
        uint8_t oState  : 1;    //Pin initial output state
    };

    constexpr static PinCfg pinConf[] =
    {
        {Port::PORTA,  1, Mode::AF,     Speed::LS,  7, 0},   //RS485_DE
        {Port::PORTA,  2, Mode::AF,     Speed::LS,  7, 0},   //RS485_TX
        {Port::PORTA,  3, Mode::AF,     Speed::LS,  7, 0},   //RS485_RX

        {Port::PORTA,  4, Mode::OUTPUT, Speed::LS,  0, 0},   //WZ_RST_N - initial hold in reset
        {Port::PORTB, 12, Mode::INPUT,  Speed::LS,  0, 0},   //WZ_INT_N
        {Port::PORTB, 10, Mode::OUTPUT, Speed::VHS, 0, 1},   //WZ_CS_N  - initial deselected
        {Port::PORTA,  5, Mode::AF,     Speed::VHS, 5, 0},   //WZ_SCK
        {Port::PORTA,  6, Mode::AF,     Speed::VHS, 5, 0},   //WZ_MISO
        {Port::PORTA,  7, Mode::AF,     Speed::VHS, 5, 0},   //WZ_MOSI
        {Port::PORTA,  8, Mode::AF,     Speed::VHS, 0, 0},   //WZ_CLK

        {Port::PORTB, 15, Mode::OUTPUT, Speed::LS,  0, 0},   //EEP_CS
        {Port::PORTB,  1, Mode::AF,     Speed::LS,  4, 0},   //EEP_SCK
        {Port::PORTB,  0, Mode::AF,     Speed::LS,  5, 0},   //EEP_MISO
        {Port::PORTB,  2, Mode::AF,     Speed::LS,  7, 0},   //EEP_MOSI
        {Port::PORTB, 13, Mode::OUTPUT, Speed::LS,  0, 0},   //EEP_ORG
        {Port::PORTB, 14, Mode::OUTPUT, Speed::LS,  0, 0},   //EEP_PE

        {Port::PORTA,  9, Mode::AF,     Speed::LS,  7, 0},   //SER_TX
        {Port::PORTA, 10, Mode::AF,     Speed::LS,  7, 0},   //SER_RX

        {Port::PORTA, 11, Mode::AF,     Speed::LS,  9, 0},   //CAN_RX
        {Port::PORTA, 12, Mode::AF,     Speed::LS,  9, 0},   //CAN_TX

        {Port::PORTA, 13, Mode::AF,     Speed::HS,  0, 0},   //SWDIO
        {Port::PORTA, 14, Mode::AF,     Speed::HS,  0, 0},   //SWCLK
        {Port::PORTB,  3, Mode::AF,     Speed::HS,  0, 0},   //SWO

        {Port::PORTH,  1, Mode::OUTPUT, Speed::LS,  0, 0},   //LED

        {Port::PORTA,  0, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PA0
        {Port::PORTA, 15, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PA15
        {Port::PORTB,  4, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PB4
        {Port::PORTB,  5, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PB5
        {Port::PORTB,  6, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PB6
        {Port::PORTB,  7, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PB7
        {Port::PORTB,  8, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PB8
        {Port::PORTC, 13, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PC13
        {Port::PORTC, 14, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PC14
        {Port::PORTC, 15, Mode::ANALOG, Speed::LS,  0, 0},   //IO_PC15
    };

    constexpr static GPIO_TypeDef* ports[] =
    {
        GPIOA,
        GPIOB,
        GPIOC,
        GPIOH
    };

    enum class ConfField{
        ModeReg,
        SpeedReg,
        AflReg,
        AfhReg,
        OdReg
    };

    constexpr static uint32_t getReg(Port port, ConfField field)
    {
        uint32_t reg = 0;

        for(PinCfg p : pinConf)
        {
            if(p.port == port)
            {
                switch(field)
                {
                    case ConfField::ModeReg:
                        reg = BitsAndFields::writeBitField(reg, static_cast<uint8_t>(p.mode), 2, p.pinPos);
                        break;

                    case ConfField::SpeedReg:
                        reg = BitsAndFields::writeBitField(reg, static_cast<uint8_t>(p.speed), 2, p.pinPos);
                        break;

                    case ConfField::AflReg:
                    {
                        uint8_t af = static_cast<uint8_t>(p.af);

                        if(af < 8)
                        {
                            reg = BitsAndFields::writeBitField(reg, af, 4, p.pinPos);
                        }
                        break;
                    }

                    case ConfField::AfhReg:
                    {
                        uint8_t af = static_cast<uint8_t>(p.af);

                        if(af >= 8)
                        {
                            reg = BitsAndFields::writeBitField(reg, af, 4, p.pinPos);
                        }
                        break;
                    }

                    case ConfField::OdReg:
                        reg = BitsAndFields::writeBitField(reg, p.oState, 1, p.pinPos);
                        break;
                }
            }
        }

        return reg;
    }
};

extern BoardPins boardPins;

#endif //STM32H533RETx
#endif //BOARD_SETTINGS_H
