
#include "hwtimer.h"

bool ALWAYS_INLINE IS_ADVANCED(volatile TIM_t *t) {
    return t == &TIM1 || t == &TIM8;
}

void HwTimer::init(uint16_t period_, uint16_t prescaler) const
{
    if (tim == &TIM1) {
        RCC.APB2ENR |= (1 << 11); // TIM1 enable
        RCC.APB2RSTR = (1 << 11); // TIM1 reset
        RCC.APB2RSTR = 0;

    } else if (tim >= &TIM2 && tim <= &TIM7) {
        unsigned bit = 1 << ((((uintptr_t)tim) >> 10) & 7);
        RCC.APB1ENR |= bit; // TIM2-TIM7 enable
        RCC.APB1RSTR = bit; // TIM2-TIM7 reset
        RCC.APB1RSTR = 0;
    }

    // Timer configuration
    tim->PSC  = prescaler;
    tim->ARR  = period_;

    if (IS_ADVANCED(tim)) {
        tim->BDTR = (1 << 15);  // MOE - main output enable
    }

    tim->DIER = 0;
    tim->CR2  = 0;
    tim->CCER = 0;
    tim->EGR  = 1;
    tim->SR   = 0;              // clear status register
    tim->CR1  = (1 << 7) |      // ARPE - auto reload preload enable
                (1 << 2) |      // URS - update request source
                (1 << 0);       // EN - enable
}

void HwTimer::deinit() const
{
    tim->CR1  = 0;  // control disabled
    tim->DIER = 0;  // IRQs disabled
    tim->SR   = 0;  // clear status

    if (tim == &TIM2) {
        RCC.APB1ENR &= ~(1 << 0); // TIM2 disable
    }
    else if (tim == &TIM3) {
        RCC.APB1ENR &= ~(1 << 1); // TIM3 disable
    }
    else if (tim == &TIM4) {
        RCC.APB1ENR &= ~(1 << 2); // TIM4 disable
    }
    else if (tim == &TIM5) {
        RCC.APB1ENR &= ~(1 << 3); // TIM5 disable
    }
    else if (tim == &TIM1) {
        RCC.APB2ENR &= ~(1 << 11); // TIM1 disable
    }
}

// channels are numbered 1-4
void HwTimer::configureChannelAsOutput(int ch, Polarity polarity, TimerMode timmode, OutputMode outmode, DmaMode dmamode) const
{
    uint8_t mode, pol;
    (void)dmamode;

    mode = (1 << 3) |       // OCxPE - output compare preload enable
           (timmode << 4);  // OCxM  - output compare mode
    if (ch <= 2) {
        tim->CCMR1 |= mode << ((ch - 1) * 8);
    }
    else {
        tim->CCMR2 |= mode << ((ch - 3) * 8);
    }

    pol = ((unsigned)polarity << 1);        // enable OCxE (and OCxP based on polarity)
    if (IS_ADVANCED(tim) && outmode == ComplementaryOutput) {
        pol |= 1 << 2;                      // enable OCxNE
    }

    tim->compareCapRegs[ch - 1].CCR = 0;
    tim->CCER |= (pol << ((ch - 1) * 4));
}

void HwTimer::configureChannelAsInput(int ch, InputCaptureEdge edge, uint8_t filterFreq, uint8_t prescaler) const
{
    uint8_t mode =  (1 << 0) |       // configured as input, mapped on TI1
                    ((filterFreq & 0xF) << 4) |
                    ((prescaler & 0x3) << 2);

    if (ch <= 2) {
        tim->CCMR1 |= mode << ((ch - 1) * 8);
    }
    else {
        tim->CCMR2 |= mode << ((ch - 3) * 8);
    }

    tim->CCER |= edge << ((ch - 1) * 4);
}
