/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 * Some minor C++ modifications.
 */

#ifndef _SPI_HPP_
#define _SPI_HPP_

#include "variant.h"
#include <stdio.h>

enum SPITransferMode {
    SPI_CONTINUE,
    SPI_LAST
};

class SPIClass 
{
public:
    SPIClass (Spi *_spi, uint32_t _id, uint8_t _pin, uint8_t _dma);

    byte transfer (uint8_t _data, SPITransferMode _mode = SPI_LAST);
    byte transferBuffer (uint8_t *data, uint16_t length);

    byte waitForDMA ();
    void sendBufferDMA (const uint8_t *_data, uint16_t _length);

    void begin ();
    void end ();

protected:
    Spi *spi;
    uint32_t id;
    uint8_t pin;
    uint8_t dma;
    bool initialized;
};

extern SPIClass SPI;

#endif // _SPI_HPP_
