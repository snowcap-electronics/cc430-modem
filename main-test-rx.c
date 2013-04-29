/*
 * Chip test, receiver side. Toggle a led when message received.
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
#include "common.h"

#include "adc.h"
#include "i2c.h"
#include "led.h"
#include "rf.h"
#include "timer.h"
#include "tmp275.h"
#include "uart.h"
#include "utils.h"

#include "msp430.h"
#include "RF1A.h"
#include "hal_pmm.h"

#include <stdint.h>

/*
  Toggle a led on P1.6 when any message is received
*/

int main(void)
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  // Increase PMMCOREV level to 2 for proper radio operation
  SetVCore(2);

  rf_init();

  // Initialize all ports to output low
  P1OUT = 0x00;
  P1DIR = 0xFF;
  P2OUT = 0x00;
  P2DIR = 0xFF;
  P3OUT = 0x00;
  P3DIR = 0xFF;
  P5OUT = 0x00;
  P5DIR = 0xFF;
  PJOUT = 0x00;
  PJDIR = 0xFF;

  led_init();

#if SC_USE_SLEEP == 0
  // Enable interrupts
  __bis_status_register(GIE);
#endif

  while (1) {

    // If not sending nor listening, start listening
    if(!rf_transmitting && !rf_receiving) {
      unsigned char RxStatus;

      rf_receive_off();

      // Reset radio on error
      if (rf_error) {
        rf_init();
      }

      // Wait until idle
      while (((RxStatus = Strobe(RF_SNOP)) & CC430_STATE_MASK) != CC430_STATE_IDLE) {
        busysleep_ms(1);
      }

      // Start listening
      rf_receive_on();
    }

#if SC_USE_SLEEP == 1
    // Sleep while waiting for interrupt
    __bis_status_register(LPM0_bits + GIE);
#else
    busysleep_ms(1);
#endif

  }

  return 0;
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

