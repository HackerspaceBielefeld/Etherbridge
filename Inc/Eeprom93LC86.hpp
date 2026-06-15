/*
 * Eeprom93LC86.hpp
 *
 * Driver for the Microchip 93LC86 (93XX86C) 16-Kbit Microwire serial EEPROM.
 *
 * The 93LC86 is a 3-wire Microwire device, NOT plain SPI. We drive it over a
 * standard SPI master (MODE_0, MSB first) by exploiting two properties:
 *   - The device ignores leading zeros on DI until it sees the Start bit
 *     (first DI=1 while CS is high), so commands are padded to byte boundaries.
 *   - On READ, a dummy zero bit precedes the data, so the read-back data is
 *     shifted by one bit and must be re-aligned.
 *
 * This driver uses the x8 organization (ORG pin low): 2048 bytes, 11-bit
 * addresses. Write protection (PE pin) is only released around write cycles.
 *
 *  Created on: Jun 15, 2026
 *      Author: Fki
 */

#ifndef EEPROM93LC86_HPP_
#define EEPROM93LC86_HPP_

#include <cstdint>
#include <cstddef>
#include "SPI_Channel.h"
#include "FastIo.hpp"
#include "BoardPins.hpp"

class Eeprom93LC86
{
public:
    static constexpr uint16_t SIZE     = 2048;   // bytes in x8 organization
    static constexpr uint16_t ADDR_MASK = 0x7FF; // 11 address bits

    Eeprom93LC86(SPI_Channel & spi, BoardPins::Pin orgPin, BoardPins::Pin pePin)
    : spi(spi), org(orgPin), pe(pePin)
    {}

    // Select x8 organization (ORG low) and keep the array write-protected
    // (PE low). The GPIOs themselves are configured by BoardPins::init().
    void init(void);

    uint8_t readByte(uint16_t addr);
    void    readBytes(uint16_t addr, uint8_t * dst, uint16_t len);

    // Write a single byte / a block. Handles EWEN, the PE pin and waiting for
    // the self-timed write cycle. Returns false if an address is out of range.
    bool writeByte(uint16_t addr, uint8_t data);
    bool writeBytes(uint16_t addr, const uint8_t * src, uint16_t len);

    // Writes a known pattern to a scratch area, reads it back and restores
    // nothing (scratch area only). Returns true if all bytes matched.
    bool selfTest(void);

private:
    // 93LC86 opcodes (the 2 bits following the Start bit).
    enum Opcode : uint8_t { OP_MODE = 0b00, OP_WRITE = 0b01, OP_READ = 0b10, OP_ERASE = 0b11 };

    void cmdEwen(void);   // erase/write enable
    void cmdEwds(void);   // erase/write disable
    void writeByteRaw(uint16_t addr, uint8_t data);
    void waitWriteCycle(void);

    // Run one full-duplex transfer (begin -> shift -> end) over the channel.
    void xfer(uint8_t * buf, size_t len);

    SPI_Channel & spi;
    FastIo org;
    FastIo pe;
};

#endif /* EEPROM93LC86_HPP_ */
