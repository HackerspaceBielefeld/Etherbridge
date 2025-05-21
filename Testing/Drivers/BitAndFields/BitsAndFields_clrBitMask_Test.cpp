/*
 * BitsAndFields_clrBitMask_Test.cpp
 *
 *  Created on: May 14, 2025
 *      Author: Fki
 */

#include <BitsAndFields.hpp>
#include "gtest/gtest.h"


using namespace BitsAndFields;

//Tests for clrBitMask-function


//Generate mask with cleared bit at pos 0.
TEST(ClrBitMask, ClrBit0)
{
    EXPECT_EQ(clrBitMask(1, 0), 0xFFFFFFFE);
}

//Generate mask with cleared bit at pos 8.
TEST(ClrBitMask, ClrBit8)
{
    EXPECT_EQ(clrBitMask(1, 8), ~0x100);
}

//Generate mask with cleared bit at pos 32.
//Bordercase test, there should no cleared bit in mask!
TEST(ClrBitMask, ClrBit32)
{
    EXPECT_EQ(clrBitMask(1, 32), 0xFFFFFFFF);
}

//Generate mask with 4 cleared bits at pos 0.
TEST(ClrBitMask, Clr4BitsAt0)
{
    EXPECT_EQ(clrBitMask(4, 0), 0xFFFFFFF0);
}

//Generate mask of len 0. All bits should be set
TEST(ClrBitMask, Clr0Bits)
{
    EXPECT_EQ(clrBitMask(0, 0), 0xFFFFFFFF);
}

//Generate mask with 4 cleared bits at pos 16.
TEST(ClrBitMask, Clr4BitsAt16)
{
    EXPECT_EQ(clrBitMask(4, 16), ~0xF0000);
}

//Generate mask with 4 clear bits at pos 32.
//Bordercase test, there should no cleared bits in mask!
TEST(ClrBitMask, Clr4BitsAt32)
{
    EXPECT_EQ(clrBitMask(4, 32), 0xFFFFFFFF);
}

//Generate mask with 4 cleared bits at pos 30.
//Bordercase test, there should only bits 30 and 31 be cleared in mask!
TEST(ClrBitMask, Clr4BitsAt30)
{
    EXPECT_EQ(clrBitMask(4, 30), 0x3FFFFFFF);
}

//Generate mask with 32 cleared bits at pos 0.
//Bordercase test, there should all 32 bits be cleared in mask!
TEST(ClrBitMask, Clr32BitsAt0)
{
    EXPECT_EQ(clrBitMask(32, 0), 0);
}

//Generate mask with 40 cleared bits at pos 0.
//Bordercase test, there should all 32 bits be cleared in mask!
TEST(ClrBitMask, Clr40BitsAt0)
{
    EXPECT_EQ(clrBitMask(40, 0), 0);
}

//Generate mask with 40 cleared bits at pos 8.
//Bordercase test, there should the upper 24 bits be cleared in mask!
TEST(ClrBitMask, Clr40BitsAt8)
{
    EXPECT_EQ(clrBitMask(40, 8), 0xFF);
}
