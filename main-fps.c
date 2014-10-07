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
#include "led.h"
#include "fps.h"
#include "uart.h"
#include "utils.h"

#include "msp430.h"
#include "RF1A.h"
#include "hal_pmm.h"

#include <stdint.h>

/*

The wiring for this test is as follows:

 3.3V - LDR----
              |
 A3 ----------+
              |
 GND - 10kR --

The relative size of the LDR and normal resistor calibrates the level A0 reads.

LDR ohms on my 22" IPS monitor:
- white xterm: 7k
- black xterm: 45k
*/

int main(void)
{
  uint8_t led_count = 0;
  uint32_t timestamp_counter = 0;
  uint32_t timestamp_last_frame = 0;
  uint8_t channels[1] = {ADC12INCH_3};
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
  P2DIR &= ~BIT3;

  led_init();
  uart_init();

  #if SC_USE_SLEEP == 0
  // Enable interrupts
  __bis_status_register(GIE);
  #endif

  // Initiate channel A3 measurement @ 1000 Hz
  adc_start(sizeof(channels), channels, ADC12SHT03 | ADC12SHT02, ADC_MODE_CONT);

  while(1) {
    uint8_t fps;
    uint16_t missed_adc;
    uint16_t adc_value;
    uint32_t timestamp_ms;
    uint16_t low;
    uint16_t low_limit;
    uint16_t high_limit;
    uint16_t high;

    // Wait for the ADC interrupt
    while (adc_state != ADC_STATE_DATA) {}

    adc_get_data(1, &adc_value, &timestamp_ms);

    ++timestamp_counter;
    missed_adc += timestamp_ms - timestamp_counter;

    led_count += timestamp_ms - timestamp_counter;
    if (led_count++ > 100) {
      led_count = 0;
      led_toggle(1);
    }

    timestamp_counter = timestamp_ms;

#if 0
    {
      uint8_t buf[64];
      uint8_t len = 0;

      len += sc_itoa(missed_adc, &buf[len], 64 - len);
      buf[len++] = '\r';
      buf[len++] = '\n';

      uart_tx_append_msg(buf, len);
      uart_send_next_msg();
    }
#endif

    if (handle_adc(adc_value, timestamp_ms, &fps, &low, &low_limit, &high_limit, &high)) {
      uint8_t buf[64];
      uint8_t len;

      len = create_message(buf, 64,
                           timestamp_ms,
                           fps,
                           missed_adc,
                           (uint16_t)(timestamp_ms - timestamp_last_frame),
                           low,
                           low_limit,
                           high_limit,
                           high);
      uart_tx_append_msg(buf, len);
      uart_send_next_msg();

      timestamp_last_frame = timestamp_ms;
      missed_adc = 0;
    }
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

