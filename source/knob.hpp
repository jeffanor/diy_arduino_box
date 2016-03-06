/// @file knob.hpp
/// @brief Quadrature decoding of a rotary encoder with push button

#ifndef _KNOB_HPP_
#define _KNOB_HPP_

#include "hardware.h"

/// @brief Class supporting a rotary encoder with push button
class CKnob {
public:
    enum knob_pending_bit_t {
        BIT_LEFT  = 0b001,   // knob has been turned left
        BIT_RIGHT = 0b010,   // knob has been turned right
        BIT_PUSH  = 0b100    // knob has been pushed or released
    }; 

public:
    /// @brief Return the singleton CKnob object
    /// @return Pointer to the singleton CKnob object
    static CKnob *get () { return &s_singleton; }
    
    /// @brief Configure the knob pins
    /// @param pinqa    Rotary encoder A pin
    /// @param pinqb    Rotary encoder B pin
    /// @param pinpush  Rotary encoder push button pin
    void configure (uint8_t pinqa, uint8_t pinqb, uint8_t pinpush);

    /// @brief Get the push button state and relative knob movement since last query
    /// @param pressed  Pointer to receive the knob's pressed state
    /// @param relative Pointer to receive the knob's relative knob position
    /// @return         Bit mask composed from pending BIT_xxx values
    uint8_t query (uint8_t *pressed, int32_t *relative);    

protected:
    /// @brief Default constructor
    CKnob ();

    static void knob_interrupt ();
    void interrupt ();

protected:
    static CKnob s_singleton;    ///< The singleton knob object

    Pio *pioa;                   ///< The rotary encoder A pin PIO object
    Pio *piob;                   ///< The rotary encoder B pin PIO object
    Pio *piop;                   ///< The push button pin PIO object
    uint32_t maska;              ///< The rotary encoder A pin bit mask
    uint32_t maskb;              ///< The rotary encoder B pin bit mask
    uint32_t maskp;              ///< The rotary encoder push button pin bit mask

    volatile uint8_t m_ahigh;    ///< ISR internal only
    volatile uint8_t m_bhigh;    ///< ISR internal only
    volatile int32_t m_rel;      ///< relative knob position since last query()
    volatile uint8_t m_down;     ///< whether knob is down or not
    volatile uint8_t m_pending;  ///< whether a knob reading has changed
};

#endif // _KNOB_HPP_
