/// @file knob.cpp
/// @brief Quadrature decoding of a rotary encoder with push button

#include "knob.hpp"
#include "ST7735.hpp"

CKnob CKnob::s_singleton;

void
CKnob::knob_interrupt ()
{
    CKnob::get ()->interrupt ();
}

void
CKnob::interrupt ()
{
    if (((pioa->PIO_PDSR & maska) != 0) != m_ahigh) {
        m_ahigh = !m_ahigh;
        // adjust counter - 1 if A leads B
        if (!(m_pending & BIT_LEFT) && (m_ahigh && !m_bhigh)) {
            m_rel -= 1;
            m_pending |= BIT_LEFT;
        }
    }
    if (((piob->PIO_PDSR & maskb) != 0) != m_bhigh) {
        m_bhigh = !m_bhigh; 
        // adjust counter + 1 if B leads A
        if (!(m_pending & BIT_RIGHT) && (m_bhigh && !m_ahigh)) {
            m_rel += 1;
            m_pending |= BIT_RIGHT;
        }
    }
    if (((piop->PIO_PDSR & maskp) != 0) == m_down) { // == sic! down is inverted
        m_down = !m_down;
        if (m_down)
            m_pending |= BIT_PUSH;
    }
}

void
CKnob::configure (uint8_t pinqa, uint8_t pinqb, uint8_t pinpush)
{
    pioa = digitalPinToPort (pinqa);
    piob = digitalPinToPort (pinqb);
    piop = digitalPinToPort (pinpush);
    maska = digitalPinToBitMask (pinqa);
    maskb = digitalPinToBitMask (pinqb);
    maskp = digitalPinToBitMask (pinpush);

    pinMode (pinqa, INPUT_PULLUP);
    digitalWrite (pinqa, HIGH);
    attachInterrupt (pinqa, CKnob::knob_interrupt, CHANGE);
    pioa->PIO_IFER = maska;
    pioa->PIO_DIFSR = maska;
    pioa->PIO_SCDR = 20;    // SLOWCLK/20 = 1.6 kHz

    pinMode (pinqb, INPUT_PULLUP);
    digitalWrite (pinqb, HIGH);
    attachInterrupt (pinqb, CKnob::knob_interrupt, CHANGE);
    piob->PIO_IFER = maskb;
    piob->PIO_DIFSR = maskb;
    piob->PIO_SCDR = 20;    // SLOWCLK/20 = 1.6 kHz

    pinMode (pinpush, INPUT_PULLUP);
    digitalWrite (pinpush, HIGH);
    attachInterrupt (pinpush, CKnob::knob_interrupt, CHANGE);
    piop->PIO_IFER = maskp;
    piop->PIO_DIFSR = maskp;
    piop->PIO_SCDR = 20;    // SLOWCLK/20 = 1.6 kHz

    m_pending = 0;
}

uint8_t 
CKnob::query (uint8_t *out_pressed, int32_t *out_relative)
{
    if (m_pending == 0) return m_pending;
    if ((m_pending & BIT_PUSH) && out_pressed)
        *out_pressed = 1;
    if ((m_pending & (BIT_LEFT|BIT_RIGHT)) && out_relative)
        *out_relative = m_rel;
    m_rel = 0;
    uint8_t pending = m_pending;
    m_pending = 0;
    return pending;
}

CKnob::CKnob ()
: pioa (0), piob (0), piop (0), maska (0), maskb (0), maskp (0),
  m_ahigh (0), m_bhigh (0), m_rel (0), m_down (0), m_pending (0)
{
}
