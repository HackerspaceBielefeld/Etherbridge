
#include "SPI_Master.h"

  void SPI_Master::setConfig(SPI_Config const * const cfg)
  {
    setConfig(cfg->preScaler, cfg->mode, cfg->bitOrder);
  }

  void SPI_Master::setConfig(
      SPI_Master::Prescaler pre,
      SPI_Master::SPI_Mode mode,
      SPI_Master::BitOrder order
      )
  {
    //Registerwerte vorbereiten
    const uint32_t tmpCFG1 = (static_cast<uint32_t>(pre) << SPI_CFG1_MBR_Pos) | (7 << SPI_CFG1_DSIZE_Pos);
    const uint32_t tmpCFG2 = SPI_CFG2_AFCNTR | SPI_CFG2_SSM | (static_cast<uint32_t>(mode) << SPI_CFG2_CPHA_Pos)
                           | (static_cast<uint32_t>(order) << SPI_CFG2_LSBFRST_Pos) | SPI_CFG2_MASTER;

    SPIx->CR1  &= ~(SPI_CR1_IOLOCK);
    SPIx->CR1  |= SPI_CR1_SSI;
    SPIx->CFG1 |= tmpCFG1;
    SPIx->CFG2 |= tmpCFG2;
    SPIx->CR1  |= SPI_CR1_IOLOCK;
  }

  void SPI_Master::reset()
  {
    if(SPIx == SPI1)
    {
      RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
      __DSB();
      RCC->APB2RSTR &= ~(RCC_APB2RSTR_SPI1RST);
    }
    else if(SPIx == SPI3)
    {
      RCC->APB1LRSTR |= RCC_APB1LRSTR_SPI3RST;
      __DSB();
      RCC->APB1LRSTR &= ~(RCC_APB1LRSTR_SPI3RST);
    }
    __DSB();

    closeTransfer();
    unlock();
  }

  void SPI_Master::enable()
  {
    SPIx->CR1 |= SPI_CR1_SPE;
    SPIx->CR1 |= SPI_CR1_CSTART;
  }

  void SPI_Master::disable()
  {
    SPIx->CR1 &= ~(SPI_CR1_SPE);
  }

  ErrorStatus SPI_Master::write(const uint8_t data)
  {
    if(isBusy())
    {
      return ERROR;
    }

    //Achtung: Umcasten des Registerzugriffes auf 8-Bit nötig, da sonst
    //automatische DataPacking auf 16 Bit greift!
    volatile uint8_t * spiDr = reinterpret_cast<volatile uint8_t *>(&SPIx->TXDR);
    *spiDr = data;

    return SUCCESS;
  }
  
  ErrorStatus SPI_Master::write(uint8_t const * const data, const size_t len)
  {
    if(isBusy())
    {
      return ERROR;
    }

    while(isAvail() == SUCCESS)
    {
      read();
    }

    dBuf = const_cast<uint8_t *>(data);
    dataLen = len;

    noRx = true;
    busy = true;

    SPIx->IER |= SPI_IER_RXPIE | SPI_IER_TXPIE;
    return SUCCESS;
  }

  uint8_t SPI_Master::read()
  {
    uint32_t status = SPIx->SR;

    if(status & SPI_SR_RXPLVL)
    {
      return static_cast<uint8_t>(SPIx->RXDR);
    }
    else
    {
      return 0;
    }
  }

  ErrorStatus SPI_Master::read(uint8_t * const data, const size_t len)
  {
      if(isBusy())
      {
        return ERROR;
      }

      while(isAvail() == SUCCESS)
      {
        read();
      }

      dBuf = data;
      dataLen = len;

      noTx = true;
      busy = true;

      SPIx->IER |= SPI_IER_RXPIE | SPI_IER_TXPIE;
      return SUCCESS;
    }

  ErrorStatus SPI_Master::isAvail()
  {
    uint32_t status = SPIx->SR;

    if(busy || (!(status & SPI_SR_RXPLVL)))
    {
      return ERROR;
    }
    else
    {
      return SUCCESS;
    }
  }

  ErrorStatus SPI_Master::shift(uint8_t * const data, size_t len)
  {
    if(isBusy())
    {
      return ERROR;
    }

    while(isAvail() == SUCCESS)
    {
      read();
    }

    dBuf = data;
    dataLen = len;

    busy = true;

    SPIx->IER |= SPI_IER_RXPIE | SPI_IER_TXPIE;
    return SUCCESS;
  }

  ErrorStatus SPI_Master::dummy(size_t len)
  {
    if(isBusy())
    {
      return ERROR;
    }

    while(isAvail() == SUCCESS)
    {
      read();
    }

    dataLen = len;
    noTx = true;
    noRx = true;
    busy = true;

    SPIx->IER |= SPI_IER_RXPIE | SPI_IER_TXPIE;
    return SUCCESS;
  }

  ErrorStatus SPI_Master::tryLock(void)
  {
    if(__LDREXB(&lock))
    {
      return ERROR;
    }
    if(__STREXB(1, &lock) == 0)
    {
      __DMB();
      return SUCCESS;
    }
    return ERROR;
  }

  void SPI_Master::unlock(void)
  {
    lock = 0;
  }

  bool SPI_Master::isBusy(void)
  {
    return (busy || ((SPIx->SR & SPI_SR_TXC) == 0));
  }

  void SPI_Master::handler(void)
  {
    uint32_t status = SPIx->SR;

    if(status & SPI_SR_TXP)
    {
      if(txPos < dataLen)
      {
        //Achtung: Umcasten des Registerzugriffes auf 8-Bit nötig, da sonst
        //automatische DataPacking auf 16 Bit greift!
        volatile uint8_t * spiDr = reinterpret_cast<volatile uint8_t *>(&SPIx->TXDR);
        if(noTx)
        {
          *spiDr = SPI_Master::DUMMY_VAL;
        }
        else
        {
          *spiDr = dBuf[txPos];
        }
        txPos++;
      }
      else
      {
        SPIx->IER &= ~(SPI_IER_TXPIE);
      }
    }

    if(status & SPI_SR_RXP)
    {
      volatile uint8_t * spiDr = reinterpret_cast<volatile uint8_t *>(&SPIx->RXDR);
      if(!noRx)
      {
        dBuf[rxPos] = *spiDr;
      }
      else
      {
        uint8_t tmp = *spiDr;
      }
      rxPos++;

      //Wenn letztes Datum gelesen, Verbindung beenden.
      if(rxPos == dataLen)
      {
        SPIx->IER &= ~(SPI_IER_RXPIE);
        closeTransfer();
      }
    }
  }
