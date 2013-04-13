/*
 * Wired light sensor
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

static void send_message(uint16_t batt);

int main(void)
{
  unsigned char led_count = 0;

  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  // Increase PMMCOREV level to 2 for proper radio operation
  SetVCore(2);

  // Initialize all ports to output low
  // FIXME: implement a GPIO class
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

  // P2.3 (A3) as ADC
  P2SEL |= BIT3;

  // P2.3 (A3) as input
  P2DIR |= BIT3;

  led_init();

  #if SC_USE_SLEEP == 0
  // Enable interrupts
  __bis_status_register(GIE);
  #endif

  rf_init();
  rf_wait_for_idle();

  while(1) {

    // Read Channel A3
    adc_start(ADC12INCH_3, ADC12SHT0_3); // 12 == clk/1024, 3 == clk/32

    // Wait for the ADC interrupt
    __bis_status_register(LPM1_bits + GIE);

    if (led_count++ == 100) {
      led_count = 0;
      led_toggle(1);
    }

    send_message(adc_result);
  }
}


/*
 * Construct a message and send it over the RF
 */
static void send_message(uint16_t adc)
{
  unsigned char buf[UART_BUF_LEN];
  unsigned char len = 0;
  unsigned char rf_timeout = 0;

  // Send battery level
  len += sc_itoa(adc, &buf[len], UART_BUF_LEN - len);
  buf[len++] = '\r';
  buf[len++] = '\n';

  // Send the message
  rf_append_msg(buf, len);
  rf_send_next_msg(RF_SEND_MSG_FORCE);

    // Wait for completion of the tx, with timeout
  while (rf_transmitting) {
    if (rf_timeout++ == 20) {
      break;
    }
    timer_sleep_ms(1, LPM1_bits);
  }
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

