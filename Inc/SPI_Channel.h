/*
 * SPI_Channel.h
 *
 *  Created on: 30.04.2022
 *      Author: Fki
 */

#ifndef SPI_CHANNEL_H
#define SPI_CHANNEL_H

#include <SPI_Master.h>
#include "FastIo.hpp"
#include "BoardPins.hpp"

class SPI_Channel
{
public:

	enum class CS_Polarity
	{
		activeLow,
		activeHigh
	};

	constexpr SPI_Channel(SPI_Master * const spi, BoardPins::Pin csPin, CS_Polarity actState)
	: SPIx(spi), csPin(csPin),csPolarity(actState)
	{}

	void init(SPI_Master::SPI_Config const * cfg)
	{
		spiCfg = cfg;
	}

	/*
	 * Opens an SPI transaction: configures the peripheral, asserts CS and
	 * enables SPI. Pair every successful begin() with exactly one end().
	 *
	 * Transactions must NOT be nested: begin()/end() are not reference
	 * counted, and a nested end() would deassert CS while the outer caller
	 * still believes it holds the bus. Each leaf access (e.g. one register
	 * read/write) is expected to own its own begin()/end() pair instead.
	 *
	 * Returns ERROR if the bus is busy or already locked; callers typically
	 * spin with `while (begin() != SUCCESS);`.
	 */
	ErrorStatus begin(void)
	{
		if(SPIx->isBusy() || SPIx->tryLock() == ERROR)
		{
			return ERROR;
		}

		SPIx->setConfig(spiCfg);
		activateCs();
		SPIx->enable();
		return SUCCESS;
	}

	ErrorStatus end(void)
	{
		if(SPIx->isBusy())
		{
			return ERROR;
		}
		SPIx->disable();
		deactivateCs();
		SPIx->unlock();
		return SUCCESS;
	}

	ErrorStatus write(const uint8_t data)
	{
		return SPIx->write(data);
	}

	ErrorStatus write(uint8_t const * const data, const size_t len)
	{
		return SPIx->write(data, len);
	}

	ErrorStatus isAvail(void)
	{
		return SPIx->isAvail();
	}

	uint8_t read(void)
	{
		return SPIx->read();
	}

	ErrorStatus read(uint8_t * const data, const size_t len)
	{
		return SPIx->read(data, len);
	}

	ErrorStatus dummy(size_t len)
	{
		return SPIx->dummy(len);
	}

	ErrorStatus shift(uint8_t * const data, size_t len)
	{
		return SPIx->shift(data, len);
	}

	bool isBusy(void)
	{
		return SPIx->isBusy();
	}

	void reset(void)
	{
		deactivateCs();
		SPIx->reset();
	}

private:
	SPI_Master * const SPIx;
	SPI_Master::SPI_Config const * spiCfg;
	FastIo const csPin;
	CS_Polarity const csPolarity;

	void activateCs(void)
	{
		if(csPolarity == CS_Polarity::activeLow)
		{
			csPin.clr();
		}
		else
		{
			csPin.set();
		}
	}

	void deactivateCs(void)
	{
		if(csPolarity == CS_Polarity::activeLow)
		{
			csPin.set();
		}
		else
		{
			csPin.clr();
		}
	}
};

#endif /* SPI_CHANNEL_H */
