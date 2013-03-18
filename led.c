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

#include "led.h"


/*
 * Initialize LED pins
 */
void led_init(void)
{
  // Set up LEDs (no leds in Radio Board v1)
#if (USE_DEBUG_LEDS >= 1)
  // P1.0 (GPIO in Radio Board v1)
  DEBUG_LED1_POUT &= ~DEBUG_LED1_BIT;
  DEBUG_LED1_PDIR |= DEBUG_LED1_BIT;
#endif
#if (USE_DEBUG_LEDS == 2)
  // P1.2 (SPI in Radio Board v1)
  DEBUG_LED2_POUT &= ~DEBUG_LED2_BIT;
  DEBUG_LED2_PDIR |= DEBUG_LED2_BIT;
#endif
}



/*
 * Toggle the specified led between on/off
 */
void led_toggle(int led)
{
#ifdef USE_DEBUG_LEDS

  switch(led) {
  case 1:
#if (USE_DEBUG_LEDS >= 1)
    DEBUG_LED1_POUT ^= DEBUG_LED1_BIT;
#endif
    break;
  case 2:
#if (USE_DEBUG_LEDS == 2)
    DEBUG_LED2_POUT ^= DEBUG_LED2_BIT;
#endif
    break;
  default:
    break;
  }
#endif
}



/*
 * Turn the specified led off
 */
void led_off(int led)
{
#ifdef USE_DEBUG_LEDS

  switch(led) {
  case 1:
#if (USE_DEBUG_LEDS >= 1)
    DEBUG_LED1_POUT &= ~DEBUG_LED1_BIT;
#endif
    break;
  case 2:
#if (USE_DEBUG_LEDS == 2)
    DEBUG_LED2_POUT &= ~DEBUG_LED2_BIT;
#endif
    break;
  default:
    break;
  }
#endif
}



/*
 * Turn the specified led on
 */
void led_on(int led)
{
#ifdef USE_DEBUG_LEDS

  switch(led) {
  case 1:
#if (USE_DEBUG_LEDS >= 1)
    DEBUG_LED1_POUT |= DEBUG_LED1_BIT;
#endif
    break;
  case 2:
#if (USE_DEBUG_LEDS == 2)
    DEBUG_LED2_POUT |= DEBUG_LED2_BIT;
#endif
    break;
  default:
    break;
  }
#endif
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

