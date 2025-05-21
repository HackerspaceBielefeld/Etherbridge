/*
 * BitsAndFields_clrBitMask_Test.cpp
 *
 *  Created on: May 14, 2025
 *      Author: Fki
 */

#include <BitsAndFields.hpp>
#include "gtest/gtest.h"


using namespace BitsAndFields;

//Tests for setBit- and clrBit-functions


//Set bit at pos 0.
TEST(SetAndClrBit, SetBit0)
{
    EXPECT_EQ(setBit(0, 0), 1);
}

//Set bit at pos 4.
TEST(SetAndClrBit, SetBit4)
{
    EXPECT_EQ(setBit(0, 4), 0x10);
}

//Set bit at pos 32.
//Bordercase test, there should be no set bit!
TEST(SetAndClrBit, SetBit32)
{
    EXPECT_EQ(setBit(0, 32), 0);
}

//Clear bit at pos 0.
TEST(SetAndClrBit, ClrBit0)
{
    EXPECT_EQ(clrBit(0xFFFFFFFF, 0), 0xFFFFFFFE);
}

//Clear bit at pos 4.
TEST(SetAndClrBit, ClrBit4)
{
    EXPECT_EQ(clrBit(0xFFFFFFFF, 4), 0xFFFFFFEF);
}

//Clear bit at pos 32.
//Bordercase test, there should be no cleared bit!
TEST(SetAndClrBit, ClrBit32)
{
    EXPECT_EQ(clrBit(0xFFFFFFFF, 32), 0xFFFFFFFF);
}

//Get bit from pos 0.
TEST(SetAndClrBit, GetBit0)
{
    EXPECT_EQ(getBit(0xA, 0), 0);
    EXPECT_EQ(getBit(0x5, 0), 1);
}

//Get bit from pos 4.
TEST(SetAndClrBit, GetBit4)
{
    EXPECT_EQ(getBit(0xBA, 4), 1);
    EXPECT_EQ(getBit(0x65, 4), 0);
}

//Get bit from pos 32.
//Bordercase test, bit should be 0!
TEST(SetAndClrBit, GetBit32)
{
    EXPECT_EQ(getBit(0xFFFFFFFF, 32), 0);
}
