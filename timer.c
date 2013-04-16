/*
 * Timer functions
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

#include "timer.h"
#include "utils.h"

static uint16_t timer_repeats = 0;
uint8_t timer_occurred = 0;

/*
 * Timeout, repeat timer_repeats times, then wake up from sleep
 */
__attribute__((interrupt(TIMER1_A0_VECTOR)))
void TIMER1_A0_ISR(void)
{

  if (timer_repeats > 0) {
    --timer_repeats;
    return;
  }

  timer_clear();
#if SC_USE_SLEEP == 1
  // Exit from lower power mode
  // Clearing all LPM4 bits should be fine even if we are only in LPM0
  __bic_status_register_on_exit(LPM4_bits);
#endif
  timer_occurred = 1;
}





/*
 * Set timer to raise an interrupt after ms milliseconds
 * NOTE: this assumes timer_repeats has already been set!
 */
void timer_set(int ms)
{
  if (ms > 16000) {
    ms = 16000;
  }

  timer_occurred = 0;
  TA1CCR0  = ms << 0;                       // ms milliseconds
  TA1CTL = TASSEL_1 + MC_1 + ID_3;          // ACLK/8, upmode
  TA1CCTL0 = CCIE;                          // CCR0 interrupt enabled
}



/*
 * Set timer to raise an interrupt after ms milliseconds
 */
void timer_clear(void)
{
  // Clear timer register, disable timer interrupts
  timer_occurred = 0;
  timer_repeats = 0;
  TA1CCTL0 = 0;                             // CCR0 interrupt disabled
  TA1CTL = TACLR;
}



/*
 * Block in low power mode for ms milliseconds
 */
void timer_sleep_ms(uint16_t ms, uint32_t mode)
{
#if SC_USE_SLEEP == 1
  timer_repeats = 0;
  timer_set(ms);
  __bis_status_register(mode + GIE);
#else
  busysleep_ms(ms);
#endif
}



/*
 * Block in low poewr mode for min minutes
 */
void timer_sleep_min(uint16_t min, uint32_t mode)
{
#if SC_USE_SLEEP == 1
  // Repeat 10sec sleep 6*min times.
  timer_repeats = 6 * min;
  timer_set(10000); // sleep 10 sec timer_repeats times
  __bis_status_register(mode + GIE);
#else
  int i;
  for (i = 0; i < 6 * min; ++i) {
    busysleep_ms(10000);
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

