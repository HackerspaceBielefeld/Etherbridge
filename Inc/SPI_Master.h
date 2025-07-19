/*
 * SPI_Master.h
 *
 *  Created on: 30.04.2022
 *      Author: Fki
 */

#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#if defined(STM32H5)
    #include "stm32h5xx.h"
#else
  #error Kein unterstütztes Target!
#endif

#include <cstddef>
#include <cstdint>

class SPI_Master
{
public:
  enum class Prescaler 
  {
    DIV_2   = 0,
    DIV_4   = 1,
    DIV_8   = 2,
    DIV_16  = 3,
    DIV_32  = 4,
    DIV_64  = 5,
    DIV_128 = 6,
    DIV_256 = 7,
  };
  
  enum class SPI_Mode 
  {
    MODE_0 = 0,
    MODE_1 = 1,
    MODE_2 = 2,
    MODE_3 = 3
  };
  
  enum class BitOrder 
  {
    MSB_FIRST,
    LSB_FIRST
  };
  
  struct SPI_Config
  {
    SPI_Master::Prescaler preScaler;
    SPI_Master::SPI_Mode mode;
    SPI_Master::BitOrder bitOrder;
  };
  
  static constexpr uint8_t DUMMY_VAL = 0xFF;

  SPI_Master(SPI_TypeDef * const spi)
  : SPIx(spi), lock(false)
  {}
  

  void setConfig(
      Prescaler pre = Prescaler::DIV_256,
      SPI_Mode mode = SPI_Mode::MODE_0,
      BitOrder order = BitOrder::MSB_FIRST
      );

  void setConfig(SPI_Config const * const cfg);

  void reset(void);

  void enable(void);
  
  void disable(void);

  ErrorStatus write(const uint8_t data);
  ErrorStatus write(uint8_t const * const data, const size_t len);
  
  uint8_t read();
  ErrorStatus read(uint8_t * const data, const size_t len);
  
  ErrorStatus isAvail();
  
  ErrorStatus shift(uint8_t * const data, size_t len);
  
  ErrorStatus dummy(size_t len);

  ErrorStatus tryLock(void);
  
  void unlock(void);

  bool isBusy(void);

  void handler(void);

private:
  //SPI-HW
  SPI_TypeDef * const SPIx;

  //Datenpuffer
  volatile uint8_t * dBuf;
  volatile size_t rxPos;
  volatile size_t txPos;
  volatile size_t dataLen;

  //Steuerflags
  volatile bool noRx;
  volatile bool noTx;

  //Statusflag
  volatile bool busy;

  //Lock
  volatile uint8_t lock;

  void closeTransfer(void)
  {
    dBuf = NULL;
    rxPos = 0;
    txPos = 0;
    dataLen = 0;
    noRx = false;
    noTx = false;
    busy = false;
  }

};

#endif /* SPI_MASTER_H */
