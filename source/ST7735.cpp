/*******************************************************************************
Slimmed-down ST7735.cpp + GFX.cpp from ADAFRUIT
- specialized to red-tab displays and SAM3X8E only
- removed software spi support
- spi clock upped to 14 Mhz 
- reset delays reduced to 50 ms
- fixed rotation to '3' (hold panel horizontally with connector to the left)
- removed inversion
- improved character drawchar speed
- added drawing strings
- added rgb 332 bitmaps
- tear effect control enabled
- added a full ASCII textbuffer class with double buffering
******************************************************************************** 
This is a library for the Adafruit 1.8" SPI display.
This library works with the Adafruit 1.8" TFT Breakout w/SD card
----> http://www.adafruit.com/products/358
as well as Adafruit raw 1.8" TFT display
----> http://www.adafruit.com/products/618

Check out the links above for our tutorials and wiring diagrams
These displays use SPI to communicate, 4 or 5 pins are required to
interface (RST is optional)
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
MIT license, all text above must be included in any redistribution
*******************************************************************************/

#include <limits.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include "SPI.hpp"
#include "ST7735.hpp"

// Bits for MADCTL command
enum st7735_madctl_bits_t {
    MADCTL_MY  = 0x80,
    MADCTL_MX  = 0x40,
    MADCTL_MV  = 0x20,
    MADCTL_ML  = 0x10,
    MADCTL_RGB = 0x00,
    MADCTL_BGR = 0x08,
    MADCTL_MH  = 0x04
};

// ST7735 SPI command bytes
enum st7735_command_bytes_t {
    ST7735_NOP     = 0x00,
    ST7735_SWRESET = 0x01,
    ST7735_RDDID   = 0x04,
    ST7735_RDDST   = 0x09,

    ST7735_SLPIN   = 0x10,
    ST7735_SLPOUT  = 0x11,
    ST7735_PTLON   = 0x12,
    ST7735_NORON   = 0x13,

    ST7735_INVOFF  = 0x20,
    ST7735_INVON   = 0x21,
    ST7735_DISPOFF = 0x28,
    ST7735_DISPON  = 0x29,
    ST7735_CASET   = 0x2A,
    ST7735_RASET   = 0x2B,
    ST7735_RAMWR   = 0x2C,
    ST7735_RAMRD   = 0x2E,

    ST7735_PTLAR   = 0x30,
    ST7735_TEOFF   = 0x34,
    ST7735_TEON    = 0x35,
    ST7735_MADCTL  = 0x36,
    ST7735_COLMOD  = 0x3A,

    ST7735_FRMCTR1 = 0xB1,
    ST7735_FRMCTR2 = 0xB2,
    ST7735_FRMCTR3 = 0xB3,
    ST7735_INVCTR  = 0xB4,
    ST7735_DISSET5 = 0xB6,

    ST7735_PWCTR1  = 0xC0,
    ST7735_PWCTR2  = 0xC1,
    ST7735_PWCTR3  = 0xC2,
    ST7735_PWCTR4  = 0xC3,
    ST7735_PWCTR5  = 0xC4,
    ST7735_VMCTR1  = 0xC5,

    ST7735_RDID1   = 0xDA,
    ST7735_RDID2   = 0xDB,
    ST7735_RDID3   = 0xDC,
    ST7735_RDID4   = 0xDD,

    ST7735_GMCTRP1 = 0xE0,
    ST7735_GMCTRN1 = 0xE1,

    ST7735_PWCTR6  = 0xFC
};


// A 6x8 pixel character font
#include "font6x8H.i"
// A 16 color RGB565 palette
#include "palette.i"

// special number of commands to indicate a delay
#define DELAY 0x80

// Initialization commands and arguments for ST7735 red tab panel
const uint8_t Adafruit_ST7735::Rcmd[] = {
    22,                     // Number of commands in list:
    ST7735_SWRESET, DELAY,  //  1: Software reset, 0 args, w/delay
    150,                    //     150 ms delay
    ST7735_SLPOUT, DELAY,   //  2: Out of sleep mode, 0 args, w/delay
    255,                    //     500 ms delay special case
    ST7735_FRMCTR1, 3,      //  3: Frame rate ctrl - normal mode, 3 args:
    0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3,      //  4: Frame rate control - idle mode, 3 args:
    0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6,      //  5: Frame rate ctrl - partial mode, 6 args:
    0x01, 0x2C, 0x2D,       //     Dot inversion mode
    0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR, 1,       //  6: Display inversion ctrl, 1 arg, no delay:
    0x07,                   //     No inversion
    ST7735_PWCTR1, 3,       //  7: Power control, 3 args, no delay:
    0xA2,
    0x02,                   //     -4.6V
    0x84,                   //     AUTO mode
    ST7735_PWCTR2, 1,       //  8: Power control, 1 arg, no delay:
    0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3, 2,       //  9: Power control, 2 args, no delay:
    0x0A,                   //     Opamp current small
    0x00,                   //     Boost frequency
    ST7735_PWCTR4, 2,       // 10: Power control, 2 args, no delay:
    0x8A,                   //     BCLK/2, Opamp current small & Medium low
    0x2A,  
    ST7735_PWCTR5, 2,       // 11: Power control, 2 args, no delay:
    0x8A, 0xEE,
    ST7735_VMCTR1, 1,       // 12: Power control, 1 arg, no delay:
    0x0E,
    ST7735_INVOFF, 0,       // 13: Don't invert display, no args, no delay
    ST7735_MADCTL, 1,                    // 14: Memory access control (directions), 1 arg:
    MADCTL_MY | MADCTL_MV | MADCTL_RGB,  //     row addr/col addr, bottom to top refresh, p.61
    ST7735_COLMOD, 1,       // 15: set color mode, 1 arg, no delay:
    0x05,                   //     16-bit color
    ST7735_CASET, 4,        // 16: Column addr set, 4 args, no delay:
    0x00, 0x00,             //     XSTART = 0
    0x00, 0x7F,             //     XEND = 127
    ST7735_RASET, 4,        // 17: Row addr set, 4 args, no delay:
    0x00, 0x00,             //     XSTART = 0
    0x00, 0x9F,             //     XEND = 159
    ST7735_GMCTRP1, 16,     // 18: Gamma Control Positive
    0x02, 0x1c, 0x07, 0x12, //     table to convert colors to voltages
    0x37, 0x32, 0x29, 0x2d,
    0x29, 0x25, 0x2B, 0x39,
    0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16,     // 19: Gamma Control Negative
    0x03, 0x1d, 0x07, 0x06, //     table to convert colors to voltages
    0x2E, 0x2C, 0x29, 0x2D,
    0x2E, 0x2E, 0x37, 0x3F,
    0x00, 0x00, 0x02, 0x10,
    ST7735_NORON, DELAY,    // 20: Normal display on, no args, w/delay
    10,                     //     10 ms delay
    ST7735_DISPON, DELAY,   // 21: Main screen turn on, no args w/delay
    100,                    //     100 ms delay
    ST7735_TEON, 1,         // 22: Tear Effect Control On
    0x00
};

void 
Adafruit_ST7735::writecommand (uint8_t c) 
{
    m_rsport->PIO_CODR |= m_rspinmask;
    m_csport->PIO_CODR |= m_cspinmask;
    SPI.transfer (c);
    m_csport->PIO_SODR |= m_cspinmask;
}

void 
Adafruit_ST7735::writedata (uint8_t c) 
{
    m_rsport->PIO_SODR |= m_rspinmask;
    m_csport->PIO_CODR |= m_cspinmask;
    SPI.transfer (c);
    m_csport->PIO_SODR |= m_cspinmask;
} 

void 
Adafruit_ST7735::setAddrWindow (coord_t x0, coord_t y0, coord_t x1, coord_t y1) 
{
    writecommand (ST7735_CASET); // Column addr set
    writedata (0x00);
    writedata (x0);     // XSTART 
    writedata (0x00);
    writedata (x1);     // XEND

    writecommand (ST7735_RASET); // Row addr set
    writedata (0x00);
    writedata (y0);     // YSTART
    writedata (0x00);
    writedata (y1);     // YEND

    writecommand (ST7735_RAMWR); // write to RAM
}

Adafruit_ST7735::Adafruit_ST7735 () 
: m_csport (0),
  m_rsport (0),
  m_cspinmask (0),
  m_rspinmask (0),
  m_cs (0),
  m_rs (0),
  m_rst (0)
{
}

void
Adafruit_ST7735::configure (uint32_t cs, uint32_t rs, uint32_t rst)
{
    m_cs = cs;
    m_rs = rs;
    m_rst = rst;
    m_csport = digitalPinToPort (m_cs);
    m_rsport = digitalPinToPort (m_rs);
    m_cspinmask = digitalPinToBitMask (m_cs);
    m_rspinmask = digitalPinToBitMask (m_rs);   

    pinMode (m_rs, OUTPUT);
    pinMode (m_cs, OUTPUT);

    // SPI setup, clock at 84 MHz/6 = 14 MHz (display spec.: < 15 MHz)
    SPI.begin ();
    
    // toggle RST low to reset; CS low so it'll listen to us
    m_csport->PIO_CODR |= m_cspinmask; // Set control bits to LOW (idle)
    pinMode (m_rst, OUTPUT);
    digitalWrite (m_rst, HIGH);
    delay (500);
    digitalWrite (m_rst, LOW);
    delay (500);
    digitalWrite (m_rst, HIGH);
    delay (500);

    // boot sequence of 22 initialization commands for red tab only
    uint8_t  numCommands, numArgs;
    uint16_t ms;

    const uint8_t *addr = Rcmd;
    numCommands = *addr++;          // Number of commands to follow
    while (numCommands-- > 0) {     // For each command...
        writecommand (*addr++);     //   Read, issue command
        numArgs = *addr++;          //   Number of args to follow
        ms = numArgs & DELAY;       //   If MSB set, delay follows args
        numArgs &= ~DELAY;          //   Mask out delay bit
        while (numArgs-- > 0)       //   For each argument...
            writedata (*addr++);    //     Read, issue argument

        if (ms) {
            ms = *addr++;           // Read post-command delay time (ms)
            if (ms == 255)          // If 255, delay for 500 ms
                ms = 500;           
            delay(ms);
        }
    }

    fillScreen (BLACK);
}

void
Adafruit_ST7735::drawPixel (coord_t x, coord_t y, color_t color) 
{
    setAddrWindow (x, y, x, y);

    uint8_t hi = color >> 8, lo = color;
    m_rsport->PIO_SODR |= m_rspinmask;
    m_csport->PIO_CODR |= m_cspinmask;
    SPI.transfer (hi);
    SPI.transfer (lo);
    m_csport->PIO_SODR |= m_cspinmask;
}

void 
Adafruit_ST7735::drawVLine (coord_t x, coord_t y, coord_t h, color_t color) 
{
    setAddrWindow (x, y, x, y+h-1);

    uint8_t hi = color >> 8, lo = color;
    m_rsport->PIO_SODR |= m_rspinmask;
    m_csport->PIO_CODR |= m_cspinmask;
    while (h--) {
        SPI.transfer (hi);
        SPI.transfer (lo);
    }
    m_csport->PIO_SODR |= m_cspinmask;
}

void 
Adafruit_ST7735::drawHLine (coord_t x, coord_t y, coord_t w, color_t color) 
{
    setAddrWindow(x, y, x+w-1, y);

    uint8_t hi = color >> 8, lo = color;
    m_rsport->PIO_SODR |= m_rspinmask;
    m_csport->PIO_CODR |= m_cspinmask;
    while (w--) {
        SPI.transfer (hi);
        SPI.transfer (lo);
    }
    m_csport->PIO_SODR |= m_cspinmask;
}

void 
Adafruit_ST7735::fillScreen (color_t color) 
{
    fillRect (0, 0, ST7735_TFTWIDTH, ST7735_TFTHEIGHT, color);
}

void 
Adafruit_ST7735::fillRect (coord_t x, coord_t y, coord_t w, coord_t h, color_t color) 
{
    setAddrWindow (x, y, x+w-1, y+h-1);

    uint8_t hi = color >> 8, lo = color;
    uint16_t count;

    m_rsport->PIO_SODR |= m_rspinmask;
    m_csport->PIO_CODR |= m_cspinmask;
    for (count = h*w; count > 0; --count) {
        SPI.transfer (hi);
        SPI.transfer (lo);
    }
    m_csport->PIO_SODR |= m_cspinmask;
}

void 
Adafruit_ST7735::drawRect (coord_t x, coord_t y, coord_t w, coord_t h, color_t color) 
{
    drawHLine (x, y, w, color);
    drawHLine (x, y+h-1, w, color);
    drawVLine (x, y, h, color);
    drawVLine (x+w-1, y, h, color);
}

void 
Adafruit_ST7735::drawChar (coord_t x, coord_t y, unsigned char c, color_t fg, color_t bg) 
{
    uint8_t i, j;
    uint16_t color;
    uint8_t line;

    setAddrWindow (x, y, x+FONTWIDTH, y+FONTHEIGHT);
    m_rsport->PIO_SODR |= m_rspinmask;
    m_csport->PIO_CODR |= m_cspinmask;
    for (j = c*FONTHEIGHT; j < (c+1)*FONTHEIGHT; ++j) {
        line = font[j];
        for (i = 0; i < FONTWIDTH; ++i) {
            color = (line & 1) ? fg : bg; 
            line >>= 1;
            SPI.transfer (color >> 8);
            SPI.transfer (color);
        }
    }
    m_csport->PIO_SODR |= m_cspinmask;      
}

void 
Adafruit_ST7735::drawString (coord_t x, coord_t y, char *c, color_t fg, color_t bg) 
{
    for ( ; *c != 0; ++c, x += FONTWIDTH)
        drawChar (x, y, *c, fg, bg);
}

// =============================================================================
// TextFrameBuffer
// =============================================================================

TextFrameBuffer *TextFrameBuffer::s_singleton = 0;

TextFrameBuffer::TextFrameBuffer ()
: Adafruit_ST7735 ()
{
    if (!s_singleton)
        s_singleton = this;
    textAttr (ATTR (7, 0));
    memset (m_buf, 0x00, ST7735_SCRWIDTH*ST7735_SCRHEIGHT*2);
    dirty_clean ();
    dirty_update (0, 0, ST7735_SCRWIDTH, ST7735_SCRHEIGHT);
}

void 
TextFrameBuffer::textAttr (uint8_t at)
{
    m_at = at;
}

void 
TextFrameBuffer::putChAt (coord_t x, coord_t y, char ch, uint8_t at)
{
    m_buf[y][x][0] = ch;
    m_buf[y][x][1] = at;
    dirty_update (x, y, x+1, y+1);
}

void 
TextFrameBuffer::textOut (coord_t x, coord_t y, const char *s, uint16_t length)
{
    if ((x >= ST7735_SCRWIDTH) || (y < 0) || (y >= ST7735_SCRHEIGHT)) return;
    while ((x < 0) && (*s != 0) && (length > 0)) {
        ++x;
        ++s;
        --length;
    }
    if (length == 0 || *s == 0) return;

    coord_t x0 = x;
    while ((x < ST7735_SCRWIDTH) && (*s != 0) && (length > 0)) {
        m_buf[y][x][0] = *s;
        m_buf[y][x][1] = m_at;
        ++x;
        ++s;
        --length;
    }
    dirty_update (x0, y, x, y+1);
}

void 
TextFrameBuffer::decimalOut (coord_t x, coord_t y, uint32_t val, 
    uint16_t ndigits, uint16_t ndecimal, bool leadzero)
{
    coord_t cx = x + ndigits + ndecimal + ((ndecimal > 0) ? 1 : 0);
    coord_t cy = y;
    dirty_update (x, y, cx, y+1);
    uint8_t place = 0;
    while (place < ndigits+ndecimal) {
        m_buf[cy][cx][1] = m_at;
        m_buf[cy][cx--][0] = (val > 0 || place <= ndecimal || leadzero) 
                           ? '0' + (val % 10) : ' ';
        val /= 10;
        if (++place == ndecimal) {
            m_buf[cy][cx][1] = m_at;
            m_buf[cy][cx--][0] = '.';
        }
    }
}

void 
TextFrameBuffer::bar (coord_t x0, coord_t y0, coord_t x1, coord_t y1, char ch, uint8_t at)
{
    dirty_update (x0, y0, x1, y1);

    for (coord_t y = y0; y < y1; ++y)
        for (coord_t x = x0; x < x1; ++x) {
            m_buf[y][x][0] = ch;
            m_buf[y][x][1] = at;
        }
}

void 
TextFrameBuffer::frame (coord_t x0, coord_t y0, coord_t x1, coord_t y1)
{
    bar (x0, y0, x1, y1, ' ', m_at);

    for (coord_t x = x0; x < x1; ++x) {
        m_buf[y0][x][0] = FRAME_N1;
        m_buf[y1-1][x][0] = FRAME_S1;
    }
    for (coord_t y = y0; y < y1; ++y) {
        m_buf[y][x0][0] = FRAME_E1;
        m_buf[y][x1-1][0] = FRAME_W1;
    }
    m_buf[y0][x0][0] = FRAME_NW1;
    m_buf[y0][x1-1][0] = FRAME_NE1;
    m_buf[y1-1][x0][0] = FRAME_SW1;
    m_buf[y1-1][x1-1][0] = FRAME_SE1;

    dirty_update (x0, y0, x1, y1);
}

void 
TextFrameBuffer::hbar (coord_t x0, coord_t y0, uint8_t halfchars, uint8_t maxhalfchars)
{
    maxhalfchars -= halfchars;
    coord_t x = x0;
    while (halfchars >= 2) {
        m_buf[y0][x][0] = FULL;
        m_buf[y0][x][1] = m_at;
        halfchars -= 2;
        ++x;
    }
    if (halfchars > 0) {
        m_buf[y0][x][0] = HALF_W;
        m_buf[y0][x][1] = m_at;
        ++x;
    }
    while (maxhalfchars >= 2) {
        m_buf[y0][x][0] = EMPTY;
        m_buf[y0][x][1] = m_at;
        maxhalfchars -= 2;
        ++x;        
    }
    dirty_update (x0, y0, x, y0+1);
}


void
TextFrameBuffer::dirty_clean ()
{
    m_dirty.x0 = ST7735_SCRWIDTH;
    m_dirty.y0 = ST7735_SCRHEIGHT;
    m_dirty.x1 = 0;
    m_dirty.y1 = 0; 
}

void
TextFrameBuffer::dirty_update (coord_t x0, coord_t y0, coord_t x1, coord_t y1)
{
    x0 = constrain (x0, 0, ST7735_SCRWIDTH);
    x1 = constrain (x1, 0, ST7735_SCRWIDTH);
    y0 = constrain (y0, 0, ST7735_SCRHEIGHT);
    y1 = constrain (y1, 0, ST7735_SCRHEIGHT);
    m_dirty.x0 = min (m_dirty.x0, x0);
    m_dirty.y0 = min (m_dirty.y0, y0);
    m_dirty.x1 = max (m_dirty.x1, x1);
    m_dirty.y1 = max (m_dirty.y1, y1);
}

void 
TextFrameBuffer::render ()
{
    if ((m_dirty.x1 > m_dirty.x0) && (m_dirty.y1 > m_dirty.y0)) {
        // dma transfer buffers
        uint16_t scanline[2][ST7735_TFTWIDTH];
        uint32_t dummy;
        uint8_t nbuf = 0;
        // address dirty region
        setAddrWindow (m_dirty.x0 * FONTWIDTH, m_dirty.y0 * FONTHEIGHT, 
                       m_dirty.x1 * FONTWIDTH - 1, m_dirty.y1 * FONTHEIGHT - 1);
        m_rsport->PIO_SODR |= m_rspinmask;
        m_csport->PIO_CODR |= m_cspinmask;
        // character row
        for (coord_t y = m_dirty.y0; y < m_dirty.y1; ++y) {
            // character scanlines
            for (coord_t jj = 0; jj < FONTHEIGHT; ++jj) {
                // character pixels
                nbuf = 1-nbuf;
                uint16_t *dst = scanline[nbuf];
                for (coord_t x = m_dirty.x0; x < m_dirty.x1; ++x) {
                    uint8_t line = font[m_buf[y][x][0]*FONTHEIGHT + jj];
                    color_t fg = s_palette[m_buf[y][x][1] & 0x0f][0];
                    color_t bg = s_palette[m_buf[y][x][1] >> 4][1];
                    // character pixels
                    *dst++ = (line & 1) ? fg : bg; 
                    *dst++ = (line & 2) ? fg : bg; 
                    *dst++ = (line & 4) ? fg : bg; 
                    *dst++ = (line & 8) ? fg : bg; 
                    *dst++ = (line & 16) ? fg : bg; 
                    #if (FONTWIDTH >= 6)
                    *dst++ = (line & 32) ? fg : bg; 
                    #endif
                    #if (FONTWIDTH >= 7)
                    *dst++ = (line & 64) ? fg : bg; 
                    #endif
                    #if (FONTWIDTH >= 8)
                    *dst++ = (line & 128) ? fg : bg; 
                    #endif
                }       
                // send buffer via SPI to ST7735.
                // parallelize scanline assembly (CPU) and transfer (DMA)
                SPI.waitForDMA ();
                SPI.sendBufferDMA ((const uint8_t *) scanline[nbuf], (m_dirty.x1 - m_dirty.x0) * FONTWIDTH * 2);
            }
        }
        SPI.waitForDMA ();
        m_csport->PIO_SODR |= m_cspinmask;      
    }    
    dirty_clean ();
}
