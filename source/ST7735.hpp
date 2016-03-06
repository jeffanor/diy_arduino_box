/*******************************************************************************
Heavily modified and extended ST7735.h from ADAFRUIT
Below is the original header.
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

#ifndef _ST7735_HPP_
#define _ST7735_HPP_

#include "Arduino.h"
#include "Print.h"
#include <include/pio.h>
#include "hardware.h"

/// @brief Compose an attribute byte from 4-bit foreground and background palette indices
#define ATTR(fg,bg)     ((fg&0x0f) | ((bg&0x0f)<<4))

/// @brief Convert an RGB888 tripel to an RGB565 word
#define RGB565(r,g,b)   (((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)

typedef int16_t coord_t;    ///< Panel pixel coordinate word
typedef uint16_t color_t;   ///< Panel RGB565 color word

/// @brief A point
struct point_t {
    coord_t x, y;
};

/// @brief A rectangle containing (x0, y0) but not (x1, y1)
struct rect_t {
    coord_t x0, y0, x1, y1;
};

/// @brief Predefined RGB565 color constants
enum st7735_colorconstants_t {
    BLACK     = 0x0000,
    BLUE25    = 0x0007,
    BLUE50    = 0x000F,
    BLUE75    = 0x0017,
    BLUE      = 0x001F,
    GREEN25   = 0x01E0,
    GREEN50   = 0x03E0,
    GREEN75   = 0x05E0,
    GREEN     = 0x07E0,
    CYAN25    = 0x01E7,
    CYAN50    = 0x03EF,
    CYAN75    = 0x05F7,
    CYAN      = 0x07FF,
    RED25     = 0x3800,
    RED50     = 0x7800,
    RED75     = 0xB800,
    RED       = 0xF800,
    MAGENTA25 = 0x3807,
    MAGENTA50 = 0x780F,
    MAGENTA75 = 0xB817,
    MAGENTA   = 0xF81F,
    YELLOW25  = 0x39E0,
    YELLOW50  = 0x7BE0,
    YELLOW75  = 0xBDE0,
    YELLOW    = 0xFFE0,
    GREY25    = 0x39E7,
    GREY50    = 0x7BEF,
    GREY75    = 0xBDF7,
    WHITE     = 0xFFFF
};

/// @brief Geometric characters from 6x8 font for borders etc.
enum st7735_geochars_t {
    EMPTY     =  32,
    ARROWL    = 174,
    ARROWR    = 175,
    HASH1     = 176,
    HASH2     = 177,
    FRAME_E1  = 178,
    FRAME_NE1 = 190,
    FRAME_SW1 = 191,
    FRAME_N1  = 195,
    FRAME_S1  = 195,
    FRAME_SE1 = 216,
    FRAME_W1  = 178,
    FRAME_NW1 = 217,
    FULL      = 218,
    HALF_S    = 219,
    HALF_W    = 220,
    HALF_E    = 221,
    HALF_N    = 222,
    DOT       = 248
};

/// @brief A class to access ST7735 based TFT displays
class Adafruit_ST7735
{
public:
    /// @brief Configure the pins of the TFT display
    void configure (uint32_t cs, uint32_t rs, uint32_t rst);

    /// @brief Fill the entire screen in a solid RGB565 color
    void fillScreen (color_t color);

    /// @brief Fill a rectangle in a solid RGB565 color
    void fillRect (coord_t x, coord_t y, coord_t w, coord_t h, color_t color);

    /// @brief Draw a single pixel given coordinates and RGB565 color
    void drawPixel (coord_t x, coord_t y, color_t color);

    /// @brief Draw a vertical line in a solid RGB565 color
    void drawVLine (coord_t x, coord_t y, coord_t h, color_t color);

    /// @brief Draw a horizontal line in a solid RGB565 color
    void drawHLine (coord_t x, coord_t y, coord_t w, color_t color);


    /// @brief Outline a rectangle in a solid RGB565 color
    void drawRect (coord_t x, coord_t y, coord_t w, coord_t h, color_t color);

    /// @brief Draw a character from the 6x8 bitma font given foreground and background colors
    void drawChar (coord_t x, coord_t y, unsigned char c, color_t color, color_t bg);

    /// @brief Draw a string using the 6x8 bitmap font given foreground and background colors
    void drawString (coord_t x, coord_t y,  char *c, color_t color, color_t bg);

    /// @brief Draw a bitmap of RGB332 colors from memory
    // void drawBitmap332 (coord_t x, coord_t y, coord_t w, coord_t h, uint8_t *bitmap);

protected:
    Adafruit_ST7735 ();

    void setAddrWindow (coord_t x0, coord_t y0, coord_t x1, coord_t y1);
    void writecommand (uint8_t c);
    void writedata (uint8_t d);

    static const uint8_t Rcmd[];    ///< boot sequence commands
    static const uint8_t font[];    ///< font for drawChar()

    Pio *m_csport;
    Pio *m_rsport;
    uint32_t m_cspinmask;
    uint32_t m_rspinmask;
    uint8_t m_cs;
    uint8_t m_rs;
    uint8_t m_rst;
};

/// @brief A text framebuffer class built on Adafruit_ST7735
class TextFrameBuffer : public Adafruit_ST7735
{
public:
    /// @brief Default constructor
    TextFrameBuffer ();

    /// @brief Get the singleton pointer to the text frame buffer
    static TextFrameBuffer *get () { return s_singleton; }

    /// @brief Set the foreground and background palette index as attribute
    /// @brief attr  8-bit character attribute, @see ATTR macro
    void textAttr (uint8_t attr);

    /// @brief Put a character and attribute at a position
    /// @param x   Horizontal character coordinate
    /// @param y   Vertical character coordinate
    /// @param ch  Character to put
    /// @param at  8-bit character attribute to put, @see ATTR macro
    void putChAt (coord_t x, coord_t y, char ch, uint8_t at);

    /// @brief Draw a string to a character position without moving the cursor
    /// @param x  Horizontal character coordinate
    /// @param y  Vertical character coordinate
    void textOut (coord_t x, coord_t y, const char *s, uint16_t length = 65535);

    /// @brief Draw a scaled decimal number at the current cursor location and 
    ///        move the cursor. The value shown is val / 10^ndecimal
    /// @param x        Horizontal character coordinate
    /// @param y        Vertical character coordinate
    /// @param val      Unsigned value to draw
    /// @param ndigits  Number of pre-decimal digits; 
    ///                 must be > ceil (log10 (val) - ndecimal)
    /// @param ndecimal Number of post-decimal digits, or 0 if none
    /// @param leadzero Whether to draw leading zeros or leading blanks
    void decimalOut (coord_t x, coord_t y, uint32_t val, uint16_t ndigits, 
                     uint16_t ndecimal, bool leadzero);

    /// @brief Fill the currently set window with a character and color
    /// @param x0  Horizontal character coordinate
    /// @param y0  Vertical character coordinate
    /// @param x1  Right horizontal character coordinate
    /// @param y1  Bottom vertical character coordinate
    /// @param ch  Character to put
    /// @param at  8-bit character attribute to put, @see ATTR macro
    void bar (coord_t x0, coord_t y0, coord_t x1, coord_t y1, char ch, uint8_t at);

    /// @brief Draw a character frame
    /// @param x0  Left horizontal character coordinate
    /// @param y0  Top vertical character coordinate
    /// @param x1  Right horizontal character coordinate
    /// @param y1  Bottom vertical character coordinate
    void frame (coord_t x0, coord_t y0, coord_t x1, coord_t y1);

    /// @brief Draw a horizontal bar with a length given in half-chars
    /// @param x0            Left horizontal character coordinate
    /// @param y0            Vertical character coordinate
    /// @param halfchars     Bar length in half characters
    /// @param maxhalfchars  Maximum bar length in half characters
    void hbar (coord_t x0, coord_t y0, uint8_t halfchars, uint8_t maxhalfchars);

    /// @brief Render the text buffer to the ST7735 TFT display via DMAC
    void render ();

protected:
    void dirty_update (coord_t x0, coord_t y0, coord_t x1, coord_t y1);
    void dirty_clean ();

protected:
    static TextFrameBuffer *s_singleton;
    static color_t s_palette[16][2];        ///< 16 color palette

    uint8_t m_buf[ST7735_SCRHEIGHT][ST7735_SCRWIDTH][2];
    struct rect_t  m_dirty;
    uint8_t        m_at;
};


#endif // _ST7735_HPP_
