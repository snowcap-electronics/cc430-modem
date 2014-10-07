/*
 * Debug led functionality
 *
 * Copyright 2013 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef RB_LED_H
#define RB_LED_H

#include "common.h"
#include <msp430.h>

#define USE_DEBUG_LEDS     2                   // Amount of debug leds

#ifdef USE_DEBUG_LEDS
#if (USE_DEBUG_LEDS >= 1)
#define DEBUG_LED1_POUT    P2OUT
#define DEBUG_LED1_PDIR    P2DIR
#define DEBUG_LED1_BIT     BIT6                // P2.6, GPIO in RBv1 and v2
#endif
#if (USE_DEBUG_LEDS == 2)
#define DEBUG_LED2_POUT    P1OUT
#define DEBUG_LED2_PDIR    P1DIR
#define DEBUG_LED2_BIT     BIT4                // P1.4, SPI in RBv1 and v2
#endif
#endif

void led_init(void);
void led_toggle(int led);
void led_off(int led);
void led_on(int led);

#endif
