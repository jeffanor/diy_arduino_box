/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "SPI.hpp"

#define DMAC_WPKEY 0x50494Fu    // secret DMAC unlock key
#define SPI_TX 1                // SPI TX = DMAC HW interface 1
#define SPI_CLK_DIVIDER 2       // 42 MHz SPI clock

SPIClass::SPIClass(Spi *_spi, uint32_t _id, uint8_t _pin, uint8_t _dma)
: spi (_spi), id (_id), pin (_pin), dma (_dma), initialized (false)
{
    // Empty
}

void 
SPIClass::begin () {

    if (initialized) return;

    DMAC->DMAC_WPMR = DMAC_WPMR_WPKEY(DMAC_WPKEY); // disable DMAC write protection
    pmc_enable_periph_clk (ID_SPI0);
    pmc_enable_periph_clk (ID_DMAC);
    DMAC->DMAC_EN &= (~DMAC_EN_ENABLE);
    DMAC->DMAC_GCFG = DMAC_GCFG_ARB_CFG_FIXED;
    DMAC->DMAC_EN = DMAC_EN_ENABLE;

    PIO_Configure(
            g_APinDescription[PIN_SPI_MOSI].pPort,
            g_APinDescription[PIN_SPI_MOSI].ulPinType,
            g_APinDescription[PIN_SPI_MOSI].ulPin,
            g_APinDescription[PIN_SPI_MOSI].ulPinConfiguration);
    PIO_Configure(
            g_APinDescription[PIN_SPI_MISO].pPort,
            g_APinDescription[PIN_SPI_MISO].ulPinType,
            g_APinDescription[PIN_SPI_MISO].ulPin,
            g_APinDescription[PIN_SPI_MISO].ulPinConfiguration);
    PIO_Configure(
            g_APinDescription[PIN_SPI_SCK].pPort,
            g_APinDescription[PIN_SPI_SCK].ulPinType,
            g_APinDescription[PIN_SPI_SCK].ulPin,
            g_APinDescription[PIN_SPI_SCK].ulPinConfiguration);

    SPI_Configure (spi, id, SPI_MR_MSTR | SPI_MR_PS | SPI_MR_MODFDIS);
    SPI_Enable (spi);

    spi->SPI_CR = SPI_CR_SPIDIS; // disable SPI
    spi->SPI_CR = SPI_CR_SWRST; // reset SPI
    spi->SPI_MR = SPI_PCS (BOARD_PIN_TO_SPI_CHANNEL (pin)) | SPI_MR_MODFDIS | SPI_MR_MSTR; // no mode fault detection, set master mode
    spi->SPI_CSR[3] = SPI_CSR_SCBR (SPI_CLK_DIVIDER) | SPI_CSR_NCPHA; // mode 0, 8-bit, divider for clock
    spi->SPI_CR |= SPI_CR_SPIEN; // enable SPI

    initialized = true;
}

void 
SPIClass::end () 
{
    // Setting the pin as INPUT will disconnect it from SPI peripheral
    pinMode (BOARD_PIN_TO_SPI_PIN (pin), INPUT);
}

byte 
SPIClass::transfer (uint8_t _data, SPITransferMode _mode) 
{
    uint32_t d = _data; // | SPI_PCS(BOARD_PIN_TO_SPI_CHANNEL(pin));
    if (_mode == SPI_LAST)
        d |= SPI_TDR_LASTXFER;

    while ((spi->SPI_SR & SPI_SR_TDRE) == 0) ;
    spi->SPI_TDR = d;

    while ((spi->SPI_SR & SPI_SR_RDRF) == 0) ;
    d = spi->SPI_RDR;
    return d & 0xFF;
}

byte 
SPIClass::transferBuffer (uint8_t *data, uint16_t length)
{
    while ((spi->SPI_SR & SPI_SR_TXEMPTY) == 0) ;
    while (length-- > 0) {
        spi->SPI_TDR = *data++; // | SPI_PCS(ch);
        while ((spi->SPI_SR & SPI_SR_TDRE) == 0) ;
    }
    while ((spi->SPI_SR & SPI_SR_TXEMPTY) == 0) ;
    return spi->SPI_RDR;
}

byte SPIClass::waitForDMA ()
{
    while (DMAC->DMAC_CHSR & (DMAC_CHSR_ENA0 << dma)) ;
    while ((spi->SPI_SR & SPI_SR_TXEMPTY) == 0) ;
    return spi->SPI_RDR;    
}

void SPIClass::sendBufferDMA (const uint8_t *data, uint16_t length)
{
    // clear pending interrupts
    uint32_t dummy = DMAC->DMAC_EBCISR; 
    // DMA transfer of scanline[y&1] to SPI (HW Intf 1)
    DMAC->DMAC_CHDR = DMAC_CHDR_DIS0 << dma;
    // DMA source address
    DMAC->DMAC_CH_NUM[dma].DMAC_SADDR = (uint32_t) data;
    // DMA destination address is SPI TDR
    DMAC->DMAC_CH_NUM[dma].DMAC_DADDR = (uint32_t) &spi->SPI_TDR;
    DMAC->DMAC_CH_NUM[dma].DMAC_DSCR = 0;
    // DMA control A: transfer length a source/dest unit size
    DMAC->DMAC_CH_NUM[dma].DMAC_CTRLA = length
                                      | DMAC_CTRLA_SRC_WIDTH_BYTE 
                                      | DMAC_CTRLA_DST_WIDTH_BYTE;
    // DMA control B: mem->periph, source ++, dest fixed
    DMAC->DMAC_CH_NUM[dma].DMAC_CTRLB = DMAC_CTRLB_SRC_DSCR 
                                      | DMAC_CTRLB_DST_DSCR 
                                      | DMAC_CTRLB_FC_MEM2PER_DMA_FC 
                                      | DMAC_CTRLB_SRC_INCR_INCREMENTING 
                                      | DMAC_CTRLB_DST_INCR_FIXED;
    // DMA config: SPI TX, dest hardware handshaking
    DMAC->DMAC_CH_NUM[dma].DMAC_CFG = DMAC_CFG_DST_PER(SPI_TX)  // 1 = SPI TX
                                    | DMAC_CFG_DST_H2SEL 
                                    | DMAC_CFG_SOD 
                                    | DMAC_CFG_FIFOCFG_ALAP_CFG;
    // enable channel to start DMA transfer
    DMAC->DMAC_CHER = DMAC_CHER_ENA0 << dma;    
}

SPIClass SPI(SPI_INTERFACE, SPI_INTERFACE_ID, BOARD_SPI_DEFAULT_SS, 0);
