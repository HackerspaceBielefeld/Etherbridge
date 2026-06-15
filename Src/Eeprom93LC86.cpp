/*
 * Eeprom93LC86.cpp
 *
 *  Created on: Jun 15, 2026
 *      Author: Fki
 */

#include "Eeprom93LC86.hpp"
#include "Timer.h"

namespace
{
// Max self-timed erase/write cycle time of the 93LC86 is 5 ms (TWC, LC
// version). Use a small margin.
constexpr uint32_t WRITE_CYCLE_MS = 6;

/*
 * Build the 2-byte (16-bit) command prefix for the x8 organization, MSB first:
 *
 *   bit15 bit14 | bit13 | bit12 bit11 | bit10 .. bit0
 *    0     0    |  SB=1 |  op1   op0  | a10 a9 a8 .. a0
 *
 * The two leading zeros are ignored by the device until it detects the Start
 * bit, which byte-aligns the command. Data bytes (for WRITE) follow the prefix.
 */
inline void buildCmd(uint8_t op, uint16_t addr, uint8_t * buf)
{
    addr &= Eeprom93LC86::ADDR_MASK;
    buf[0] = 0x20                       // Start bit (bit5)
           | (uint8_t)(op << 3)         // opcode at bits 4:3
           | (uint8_t)((addr >> 8) & 0x07); // a10..a8
    buf[1] = (uint8_t)(addr & 0xFF);    // a7..a0
}
} // namespace

void Eeprom93LC86::init(void)
{
    org.clr();  // ORG low -> x8 organization (2048 x 8)
    pe.clr();   // PE low  -> entire array write-protected
}

void Eeprom93LC86::xfer(uint8_t * buf, size_t len)
{
    while (spi.begin() != SUCCESS);     // assert CS, enable SPI
    while (spi.shift(buf, len) != SUCCESS); // full-duplex transfer (in place)
    while (spi.end() != SUCCESS);       // wait for completion, deassert CS
}

uint8_t Eeprom93LC86::readByte(uint16_t addr)
{
    // 2 command bytes + 2 bytes to clock out the data. A dummy zero bit
    // precedes the data, so the 8 data bits straddle the byte boundary:
    //   buf[2] = [dummy d7 d6 d5 d4 d3 d2 d1]
    //   buf[3] = [d0    x  x  x  x  x  x  x ]
    uint8_t buf[4] = { 0, 0, 0x00, 0x00 };
    buildCmd(OP_READ, addr, buf);
    xfer(buf, 4);
    return (uint8_t)(((buf[2] & 0x7F) << 1) | (buf[3] >> 7));
}

void Eeprom93LC86::readBytes(uint16_t addr, uint8_t * dst, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        dst[i] = readByte(addr + i);
    }
}

void Eeprom93LC86::cmdEwen(void)
{
    // Mode opcode 00 with address bits a10=1, a9=1 -> Erase/Write Enable.
    uint8_t buf[2];
    buildCmd(OP_MODE, 0x600, buf);
    xfer(buf, 2);
}

void Eeprom93LC86::cmdEwds(void)
{
    // Mode opcode 00 with address bits a10=0, a9=0 -> Erase/Write Disable.
    uint8_t buf[2];
    buildCmd(OP_MODE, 0x000, buf);
    xfer(buf, 2);
}

void Eeprom93LC86::writeByteRaw(uint16_t addr, uint8_t data)
{
    uint8_t buf[3];
    buildCmd(OP_WRITE, addr, buf);
    buf[2] = data;
    xfer(buf, 3);
}

void Eeprom93LC86::waitWriteCycle(void)
{
    // The write cycle is self-timed and runs after CS is deasserted. Poll-free
    // fixed wait for simplicity and robustness (TWC <= 5 ms).
    Timer t;
    t.delay(WRITE_CYCLE_MS);
}

bool Eeprom93LC86::writeByte(uint16_t addr, uint8_t data)
{
    return writeBytes(addr, &data, 1);
}

bool Eeprom93LC86::writeBytes(uint16_t addr, const uint8_t * src, uint16_t len)
{
    if (len == 0 || (uint32_t)addr + len > SIZE)
    {
        return false;
    }

    pe.set();     // release write protection
    cmdEwen();    // enable erase/write latch

    for (uint16_t i = 0; i < len; i++)
    {
        writeByteRaw(addr + i, src[i]);
        waitWriteCycle();
    }

    cmdEwds();    // disable erase/write latch again
    pe.clr();     // re-assert write protection
    return true;
}

bool Eeprom93LC86::selfTest(void)
{
    // Use a scratch area at the very top of the array so a future config block
    // at the start is left untouched.
    const uint16_t base = SIZE - 4;
    const uint8_t pattern[4] = { 0xA5, 0x5A, 0x00, 0xFF };

    if (!writeBytes(base, pattern, 4))
    {
        return false;
    }

    uint8_t readback[4] = { 0 };
    readBytes(base, readback, 4);

    for (uint8_t i = 0; i < 4; i++)
    {
        if (readback[i] != pattern[i])
        {
            return false;
        }
    }
    return true;
}
