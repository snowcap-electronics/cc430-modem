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

#define SC_USE_SLEEP 0

#include "common.h"
#include "adc.h"
#include "led.h"
#include "timer.h"
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


static void send_message(uint16_t adc);
static uint8_t handle_adc(uint16_t adc);
#if 0
static void values_insert(uint16_t *data, uint16_t pos, uint16_t value, uint16_t size);
#endif

int main(void)
{
  uint8_t led_count = 0;
  uint16_t adc_prev = 0;

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
  uart_init();

  #if SC_USE_SLEEP == 0
  // Enable interrupts
  __bis_status_register(GIE);
  #endif

  while(1) {
    uint8_t fps;

    // Initiate channel A3 measurement @ 1000 Hz
    adc_start(ADC12INCH_3, ADC12SHT0_4);

    fps = handle_adc(adc_prev);

    // Wait until UART transmit is ready
    while (UartTxBufferLength != 0) {}

    if (fps) {
      send_message(fps);
    }

    // Wait for the ADC interrupt
    while (adc_state != ADC_STATE_DATA) {}

    adc_prev = adc_result;

    if (led_count++ > 100) {
      led_count = 0;
      led_toggle(1);
    }
  }
}


/*
 * Takes a ADC reading as a parameter and return true for a new frame
 */
static uint8_t handle_adc(uint16_t adc)
{
#define FPS_PREV_FRAMES   65
#define FPS_STATE_INIT     0
#define FPS_STATE_HIGH     1
#define FPS_STATE_LOW      2

  static uint32_t frames[FPS_PREV_FRAMES] = { 0 };
  static uint16_t frame_i = 0;
  static uint8_t  state = FPS_STATE_INIT;
  static uint32_t timestamp_ms = 0;
  static uint16_t high = 0;
  static uint16_t low = 0xffff;
  static uint16_t new_high = 0;
  static uint16_t new_low = 0xffff;

  uint16_t high_limit;
  uint16_t low_limit;
  uint16_t quarter_range;
  uint16_t i;
  uint8_t is_new_frame = 0;
  uint8_t fps = 0;

  // Assuming this function is called exactly 1000Hz
  ++timestamp_ms;

  // FIXME: Ignore zero adc for now.
  if (adc == 0) {
    return 0;
  }

  // Gather initial statistics
  if (state == FPS_STATE_INIT && timestamp_ms < 1000) {

    if (adc > high) {
      high = adc;
    }

    if (adc < low) {
      low = adc;
    }

    return 0;
  }

  // High limit = 75% of the range
  // Low limit = 25% of the range
  quarter_range = (high - low) >> 2;
  high_limit = high - quarter_range;
  low_limit = low + 2*quarter_range;

  // Check if we are in a known state after initialisation
  if (state == FPS_STATE_INIT) {
    if (adc < low_limit) {
      state = FPS_STATE_LOW;
    }
    if (adc > high_limit) {
      state = FPS_STATE_HIGH;
    }

    // Even if we are now in a valid state, we have can have a change
    // next time earliest.
    return 0;
  }

  if (state == FPS_STATE_LOW) {

    // Frame has changed if we are in a low state and adc is high
    if (adc > high_limit) {

      state = FPS_STATE_HIGH;
      is_new_frame = 1;

      if (new_low < high_limit) {
        uint16_t change;
        uint8_t negative = 0;

        // Adjust low value 1/4 towards the latest low value
        if (new_low < low) {
          negative = 1;
          change = low - new_low;
        } else {
          change = new_low - low;
        }

        change >>= 2;

        // Make sure low doesn't overflow
        if (negative) {
          if (change < low) {
            low -= change;
          }
        } else {
          // FIXME: is the following temporary register 32bit?
          if (low + change < 0xffff) {
            low += change;
          }
        }

        // Reset low for the next low state
        new_low = 0xffff;
      }

    } else {
      // Get the most lowest value during this low state
      if (adc < new_low) {
        new_low = adc;
      }
    }
  }

  if (state == FPS_STATE_HIGH) {

    // Frame has changed if we are in a high state and adc is low
    if (adc < low_limit) {
      int16_t change;

      state = FPS_STATE_LOW;
      is_new_frame = 1;

      if (new_high > low_limit) {
        uint16_t change;
        uint8_t negative = 0;

        // Adjust high value 1/4 towards the latest high value
        if (new_high < high) {
          negative = 1;
          change = high - new_high;
        } else {
          change = new_high - high;
        }

        change >>= 2;

        // Make sure high doesn't overflow
        if (negative) {
          if (change < high) {
            high -= change;
          }
        } else {
          // FIXME: is the following temporary register 32bit?
          if (high + change < 0xffff) {
            high += change;
          }
        }

        // Reset high for the next high state
        new_high = 0;
      }
    } else {
      // Get the most highest value during this high state
      if (adc > new_high) {
        new_high = adc;
      }
    }
  }

  if (!is_new_frame) {
    return 0;
  }

  frames[frame_i] = timestamp_ms;

  // Count the amount of new frames during last 1000 ms
  for (i = 0; i < FPS_PREV_FRAMES; ++i) {
    if (timestamp_ms - frames[i] < 1000) {
      ++fps;
    }
  }

  if (++frame_i == FPS_PREV_FRAMES) {
    frame_i = 0;
  }

  return fps;
}



/*
 * Construct a message and send it over the UART
 */
static void send_message(uint16_t adc)
{
  unsigned char len;

  len = sc_itoa(adc, UartTxBuffer, UART_BUF_LEN);

  UartTxBuffer[len++] = '\r';
  UartTxBuffer[len++] = '\n';
  UartTxBufferLength = len;

  // Send the first byte to Uart
  UartTxBuffer_i = 0; // Shouldn't be needed
  UCA0TXBUF = UartTxBuffer[UartTxBuffer_i];

  return;
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

