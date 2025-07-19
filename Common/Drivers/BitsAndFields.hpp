/**
* \file
*     GPIO_Driver.hpp
*
* \version
*     1.0 - First version.
*
* \date
*     13.05.2025
*
* \author
*     Florian Kiemes
*
* \brief
*     Helper functions for setting and deleting bits in bitfields of different sizes.
**/

#ifndef BITSANDFIELDS_HPP_
#define BITSANDFIELDS_HPP_

#include <cstdint>

namespace BitsAndFields{

    /**
     * Generates a bit mask with len set bits, right-aligned at pos and all other bits cleared.
     *
     * \param len       length of consecutive set bits in field.
     * \param pos       right-most position of the set bits.
     *
     * \return          generated mask.
     */
    constexpr uint32_t setBitMask(uint8_t len, uint8_t pos)
    {
        if(pos < 32)
        {
            if(len >= 32)
            {
                return (0xFFFFFFFF << pos);
            }
            else
            {
                uint32_t field = (1 << len) - 1;
                return field << pos;
            }
        }
        else
        {
            return 0;
        }

    }

    /**
     * Generates a bit mask with len cleared bits, right-aligned at pos and all other bits set.
     *
     * \param len       length of consecutive cleared bits in field.
     * \param pos       right-most position of the cleared bits.
     *
     * \return          generated mask.
     */
    constexpr uint32_t clrBitMask(uint8_t len, uint8_t pos)
    {
        return ~setBitMask(len, pos);
    }

    /**
     * Writes len bits of val into base at pos and returns modified base.
     *
     * \param base  base value containing the bitfield to be modified.
     * \param val   value of bit field to be written.
     * \param len   length of bitfield.
     * \param pos   rightmost BIT POSITION in field.
     *
     * \return      base with applied bitfield value.
     */
    constexpr uint32_t writeBits(uint32_t base, uint32_t val, uint8_t len, uint8_t pos)
    {
        if(pos >= 32)
        {
            return base;
        }

        uint32_t clrField = base & clrBitMask(len, pos);
        uint32_t prunedVal = val & setBitMask(len, 0);
        return clrField | (prunedVal << pos);
    }

    /**
     * Writes len bits of val into base at pos * len and returns modified base.
     *
     * \param base  base value containing the bitfield to be modified.
     * \param val   value of bit field to be written.
     * \param len   length of bitfield.
     * \param pos   right aligned position of BIT FIELD. The resultin BIT POSITION is pos x len!
     *
     * \return      base with applied bitfield value.
     */
    constexpr uint32_t writeBitField(uint32_t base, uint32_t val, uint8_t len, uint8_t pos)
    {
        return writeBits(base, val, len, pos * len);
    }

    /**
     * Extracts the value of len bits at pos (right aligned) from base, shifts it to pos 0 and returns it.
     *
     * \param base  base value containing the bitfield of interest.
     * \param len   length of bitfield.
     * \param pos   right-aligned position of bit field.
     *
     * \return      read out value, right alined at pos 0.
     */
    constexpr uint32_t readBits(uint32_t base, uint8_t len, uint8_t pos)
    {
        if(pos >= 32)
        {
            return 0;
        }
        return (base >> pos) & setBitMask(len, 0);
    }

    /**
     * Sets bit at given position.
     *
     * \param data  word of data containing the bit.
     * \param pos   position of bit.
     *
     * \return      word of data with set bit at given position.
     */
    constexpr uint32_t setBit(uint32_t data, uint8_t pos)
    {
        return writeBits(data, 1, 1, pos);
    }

    /**
     * Clears bit at given position.
     *
     * \param data  word of data containing the bit.
     * \param pos   position of bit.
     *
     * \return      word of data with cleared bit at given position.
     */
    constexpr uint32_t clrBit(uint32_t data, uint8_t pos)
    {
        return writeBits(data, 0, 1, pos);
    }

    /**
     * Gets bit value at given position
     *
     * \param data  word of data to get bit from.
     * \param pos   position of bit.
     *
     * \return      bit value (0x0 or 0x1)
     */
    constexpr uint32_t getBit(uint32_t data, uint8_t pos)
    {
        return readBits(data, 1, pos);
    }

}

#endif /* BITSANDFIELDS_HPP_ */
