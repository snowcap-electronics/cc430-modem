/*
 * Chip test, sender side. Toggle a leds, then send a msg.
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
#include "gpio.h"

#include "msp430.h"
#include "RF1A.h"
#include "hal_pmm.h"

#include <stdint.h>

static void send_message(uint16_t n);

int main(void)
{
  uint16_t seq = 0;
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  // Increase PMMCOREV level to 2 for proper radio operation
  SetVCore(2);

  rf_init();

  // Initialize all GPIOs to input and others to output low
  P1OUT = 0x00;
  P1DIR = 0x00;
  P2OUT = 0x00;
  P2DIR = 0x00;
  P3OUT = 0x00;
  P3DIR = 0xFF;
  P5OUT = 0x00;
  P5DIR = 0xFF;
  PJOUT = 0x00;
  PJDIR = 0xFF;

#if SC_USE_SLEEP == 0
  // Enable interrupts
  __bis_status_register(GIE);
#endif

  while (1) {
	// Pins: P1.6, P1.5, P2.3 (CN1)
	// Pins: P1.2, P1.3, P1.4, P1.7, P2.6, P1.1, P1.0, P2.0, P2.1, P2.2 (CN2)
	uint8_t ports[13] = {1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2};
	uint8_t pins[13]  = {6, 5, 3, 2, 3, 4, 7, 6, 1, 0, 0, 1, 2};
	uint8_t i;

	// Loop X times blinking leds
	for (i = 0; i < 1; ++i) {
	  uint8_t pin;
	  for (pin = 0; pin < 13; ++pin) {
		gpio_dir(ports[pin], pins[pin], GPIO_DIR_OUT);
		gpio_on(ports[pin], pins[pin]);

		busysleep_ms(500);

		gpio_off(ports[pin], pins[pin]);
		gpio_dir(ports[pin], pins[pin], GPIO_DIR_IN);

		busysleep_ms(500);
	  }
	}
	send_message(seq++);
  }

  return 0;
}


/*
 * Construct a message and send it over the RF
 */
static void send_message(uint16_t n)
{
  unsigned char buf[UART_BUF_LEN];
  unsigned char len = 0;
  uint8_t rf_timeout = 0;

  // Send battery level
  buf[len++] = 'S';
  buf[len++] = ':';
  buf[len++] = ' ';
  len += sc_itoa(n, &buf[len], UART_BUF_LEN - len);
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

