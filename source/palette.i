/// @file palette.i
/// @brief A 16 color RGB565 palette of foreground and background colors

// swap the bytes of a word
#define BSWAP(w) ((((uint16_t)w)<<8) | (((uint16_t)w)>>8))

color_t TextFrameBuffer::s_palette[16][2] = {
    { BSWAP(BLACK   ),  BSWAP(BLACK    ) }, // 0
    { BSWAP(BLUE50  ),  BSWAP(BLUE50   ) }, // 1
    { BSWAP(GREEN50 ),  BSWAP(GREEN50  ) }, // 2
    { BSWAP(CYAN50  ),  BSWAP(CYAN50   ) }, // 3
    { BSWAP(RED50   ),  BSWAP(RED50    ) }, // 4
    { BSWAP(MAGENTA50), BSWAP(MAGENTA50) }, // 5
    { BSWAP(YELLOW50),  BSWAP(YELLOW50 ) }, // 6
    { BSWAP(GREY75  ),  BSWAP(GREY75   ) }, // 7
    { BSWAP(GREY50  ),  BSWAP(GREY50   ) }, // 8
    { BSWAP(BLUE    ),  BSWAP(BLUE     ) }, // 9
    { BSWAP(GREEN   ),  BSWAP(GREEN    ) }, // 10, A
    { BSWAP(CYAN    ),  BSWAP(CYAN     ) }, // 11, B
    { BSWAP(RED     ),  BSWAP(RED      ) }, // 12, C
    { BSWAP(MAGENTA ),  BSWAP(MAGENTA  ) }, // 13, D
    { BSWAP(YELLOW  ),  BSWAP(YELLOW   ) }, // 14, E
    { BSWAP(WHITE   ),  BSWAP(WHITE    ) }  // 15, F
};
