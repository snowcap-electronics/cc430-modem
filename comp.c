/*
 * Comparator
 *
 * Copyright 2014 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#include "comp.h"
#include "led.h"

static uint32_t comp_counter;

/*
 * Comp_B interrupt
 */
__attribute__((interrupt(COMP_B_VECTOR)))
void Comp_B_ISR (void)
{
  CBINT &= ~CBIFG;              // Clear Interrupt flag
  comp_counter++;
  //  led_toggle(1);
  // FIXME: wake up based on parameter?
#if 0
#if SC_USE_SLEEP == 1
    // Exit active
    __bic_status_register_on_exit(LPM3_bits);
#endif
#endif
}



/*
 * Start compare
 */
void comp_start(void)
{
  comp_counter = 0;

  // P2.0 (A0) as ADC
  P2SEL |= BIT0;

  // P2.0 (A0) as input
  P2DIR &= ~BIT0;

  /* Setup ComparatorB */ 
  CBCTL0 |= CBIPEN + CBIPSEL_0;             // Enable V+, input channel CB0
  CBCTL1 |= CBPWRMD_2;                      // CBMRVS=0 => select VREF1 as ref when CBOUT
                                            // is high and VREF0 when CBOUT is low
                                            // Ultra low power mode
  CBCTL2 |= CBRSEL;                         // VRef is applied to -terminal
  CBCTL2 |= CBRS_1+CBREF13;                 // VREF1 is Vcc*1/4 
  CBCTL2 |= CBREF04+CBREF03;                // VREF0 is Vcc*3/4
  CBCTL2 |= CBREFACC;                       // Clocked low power/low accuracy mode
  CBCTL3 |= BIT0;                           // Input Buffer Disable @P2.0/CB0
  CBCTL1 |= CBON;                           // Turn On ComparatorB

  __delay_cycles(75);                       // Delay for shared ref to stabilize

  CBINT = CBIE;                             // Clear any errant interrupts
                                            // Enable CompB Interrupt on rising
                                            // edge of CBIFG (CBIES=0)
}


/*
 * Return interrupt count and clear it
 */
void comp_get_count(uint32_t *counter)
{
  // Disable interrupts for mutex
  __bic_status_register(GIE);

  // Return and zero the counter
  // FIXME: it seems that interrupts are triggered both for rising and
  // falling edges, so halving the value here.
  comp_counter /= 2;
  *counter = comp_counter;
  comp_counter = 0;

  // Enable interrupts
  __bis_status_register(GIE);
}

/*
 * Shutdown comparator to save power
 */
void comp_shutdown(void)
{
  // FIXME: untested
  // Disable comparator by clearing to reset values
  CBINT  = 0;
  CBCTL3 = 0;
  CBCTL2 = 0;
  CBCTL1 = 0;
  CBCTL0 = 0;
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

