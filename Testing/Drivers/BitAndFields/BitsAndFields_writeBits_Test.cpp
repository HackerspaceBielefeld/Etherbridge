/*
 * BitsAndFields_writeBits_Test.cpp
 *
 *  Created on: May 14, 2025
 *      Author: Fki
 */

#include <BitsAndFields.hpp>
#include "gtest/gtest.h"


using namespace BitsAndFields;

//Tests for writeBits-function


//Set bit 0 without modifying other bits.
TEST(WriteBits, WriteBit0To1)
{
    EXPECT_EQ(writeBits(0xAAAAAAAA, 1, 1, 0), 0xAAAAAAAB);
}

//Set bit 2 without modifying other bits.
TEST(WriteBits, WriteBit2To1)
{
    EXPECT_EQ(writeBits(0xAAAAAAAA, 1, 1, 2), 0xAAAAAAAE);
}

//Set 4 bits wide field from pos 0 to 0x9 without modifying other bits
TEST(WriteBits, Write4BitsAt0To0x9)
{
    EXPECT_EQ(writeBits(0xAAAAAAAA, 0x9, 4, 0), 0xAAAAAAA9);
}

//Set 5 bits wide field from pos 4 to 0x4 without modifying other bits
TEST(WriteBits, Write5BitsAt4To0x4)
{
    EXPECT_EQ(writeBits(0xAAAAAAAA, 0x4, 5, 4), 0xAAAAAA4A);
}

//Set 4 bits wide field from pos 4 to 0x35 without modifying other bits
//Edgecase test: only 4 bits should be modyfied!
TEST(WriteBits, Write4BitsAt4To0x35)
{
    EXPECT_EQ(writeBits(0xAAAAAAAA, 0x35, 4, 4), 0xAAAAAA5A);
}

//Set 32 bits wide field from pos 0 to 0x87654321 without modifying other bits
TEST(WriteBits, WriteAllBitsTo0x87654321)
{
    EXPECT_EQ(writeBits(0xAAAAAAAA, 0x87654321, 32, 0), 0x87654321);
}

//Set 32 bits wide field from pos 8 to 0x87654321 without modifying other bits
//Bordercase test: Only bits from pos 8 and above should be affected.
TEST(WriteBits, WriteUpperBitsTo0x654321xx)
{
    EXPECT_EQ(writeBits(0xAAAAAAAA, 0x87654321, 32, 8), 0x654321AA);
}

//Set 16 bits wide field from pos 32 to 0x4321 without modifying other bits
//Bordercase test: No bits should be modified.
TEST(WriteBits, Write0x4321ToPos32)
{
    EXPECT_EQ(writeBits(0xAAAAAAAA, 0x4321, 16, 32), 0xAAAAAAAA);
}

//
TEST(WriteBitField, Write3to2posBitFieldAt2)
{
    EXPECT_EQ(writeBitField(0x0, 3, 2, 2), 0x30);
}

