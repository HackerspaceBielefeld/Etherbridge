/*
 *  BitsAndFields_setBitMask_Test.cpp
 *
 *  Created on: May 13, 2025
 *      Author: Fki
 */

#include <BitsAndFields.hpp>
#include "gtest/gtest.h"


using namespace BitsAndFields;

//Tests for setBitMask-function

//Generate mask with set bit at pos 0.
TEST(SetBitMask, SetBit0)
{
    EXPECT_EQ(setBitMask(1, 0), 1);
}

//Generate mask with set bit at pos 8.
TEST(SetBitMask, SetBit8)
{
    EXPECT_EQ(setBitMask(1, 8), 0x100);
}

//Generate mask with set bit at pos 32.
//Bordercase test, there should no set bit in mask!
TEST(SetBitMask, SetBit32)
{
    EXPECT_EQ(setBitMask(1, 32), 0);
}

//Generate mask with 4 set bits at pos 0.
TEST(SetBitMask, Set4BitsAt0)
{
    EXPECT_EQ(setBitMask(4, 0), 0xF);
}

//Generate mask of len 0. No bit should be set
TEST(SetBitMask, Set0Bits)
{
    EXPECT_EQ(setBitMask(0, 0), 0);
}

//Generate mask with 4 set bits at pos 16.
TEST(SetBitMask, Set4BitsAt16)
{
    EXPECT_EQ(setBitMask(4, 16), 0xF0000);
}

//Generate mask with 4 set bits at pos 32.
//Bordercase test, there should no set bits in mask!
TEST(SetBitMask, Set4BitsAt32)
{
    EXPECT_EQ(setBitMask(4, 32), 0);
}

//Generate mask with 4 set bits at pos 30.
//Bordercase test, there should only bits 30 and 31 be set in mask!
TEST(SetBitMask, Set4BitsAt30)
{
    EXPECT_EQ(setBitMask(4, 30), 0xC0000000);
}

//Generate mask with 32 set bits at pos 0.
//Bordercase test, there should all 32 bits be set in mask!
TEST(SetBitMask, Set32BitsAt0)
{
    EXPECT_EQ(setBitMask(32, 0), 0xFFFFFFFF);
}

//Generate mask with 40 set bits at pos 0.
//Bordercase test, there should all 32 bits be set in mask!
TEST(SetBitMask, Set40BitsAt0)
{
    EXPECT_EQ(setBitMask(40, 0), 0xFFFFFFFF);
}

//Generate mask with 40 set bits at pos 8.
//Bordercase test, there should the upper 24 bits be set in mask!
TEST(SetBitMask, Set40BitsAt8)
{
    EXPECT_EQ(setBitMask(40, 8), 0xFFFFFF00);
}
