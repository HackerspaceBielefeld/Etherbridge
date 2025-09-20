/*
 * BitsAndFields_readBits_Test.cpp
 *
 *  Created on: May 14, 2025
 *      Author: Fki
 */

#include <BitsAndFields.hpp>
#include "gtest/gtest.h"


using namespace BitsAndFields;

//Tests for readBits-function


//Read bit 0.
TEST(ReadBits, ReadBit0)
{
    EXPECT_EQ(readBits(0x87654321, 1, 0), 1);
}

//Read 8 bits from pos 0.
TEST(ReadBits, Read8BitsFrom0)
{
    EXPECT_EQ(readBits(0x87654321, 8, 0), 0x21);
}

//Read 12 bits from pos 4
TEST(ReadBits, Read12BitsFrom4)
{
    EXPECT_EQ(readBits(0x87654321, 12, 4), 0x432);
}

//Read 32 bits from 0
TEST(ReadBits, Read32BitsFrom0)
{
    EXPECT_EQ(readBits(0x87654321, 32, 0), 0x87654321);
}

//Read 12 bits from pos 32
//Edgecase-test, should return 0
TEST(ReadBits, Read12BitsFrom32)
{
    EXPECT_EQ(readBits(0x87654321, 12, 32), 0);
}

//Read 12 bits from pos 24
//Edgecase-test, should only return 0x87
TEST(ReadBits, Read12BitsFrom24)
{
    EXPECT_EQ(readBits(0x87654321, 12, 24), 0x87);
}
