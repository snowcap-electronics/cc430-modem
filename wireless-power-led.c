/*
 * Wireless current meter
 *
 * Copyright 2014 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 * Copyright 2012 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
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
#include "comp.h"
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

static void send_message(uint16_t batt, uint16_t temp, uint32_t blinks);

#define RB_USE_RF                1
#define RB_USE_ADC               1
#define RB_USE_I2C               1
#define RB_USE_SHUTDOWN_TMP275   0

int main(void)
{
  uint8_t temp_counter = 0;

  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  // Select VLOCLK for aux clock source
  UCSCTL4 |= SELA__VLOCLK;

  SetVCore(0);

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

  //led_init();

  #if SC_USE_SLEEP == 0
  // Enable interrupts
  __bis_status_register(GIE);
  #endif
  
  #if RB_USE_I2C
  // Shutdown TMP275 to save power
  i2c_init();
  tmp275_shutdown();
  // FIXME: why i2c_shutdown increased the power consumption by 1-2mA?
  //i2c_shutdown();
  #endif

  // Start comparator to count led blinks
  comp_start();

  while(1) {
    uint16_t adcbatt = 0;
    uint16_t temp = 0;
    uint32_t blinks = 0;
    uint8_t i;

    // Wait awhile gathering blinks
#if 1
    timer_sleep_min(1, LPM4_bits);
#else
    for (i = 0; i < 100; i++) {
      led_toggle(2);
      timer_sleep_ms(100, LPM4_bits);
    }
#endif
    comp_get_count(&blinks);

    #if RB_USE_I2C
    tmp275_start_oneshot();
    timer_sleep_ms(220, LPM4_bits);
    temp = i2c_read();
    // TMP275 will shutdown after one shot conversion
    #endif

    #if RB_USE_RF
    // Increase PMMCOREV level to 2 for proper radio operation
    SetVCore(2);
    rf_init();
    #endif

    #if RB_USE_ADC
    if (1) {
      uint8_t adc_timeout = 0;

      adc_start(ADC_CHANNEL_BATTERY, ADC12SHT0_6, ADC_MODE_SINGLE);

      // FIXME: implement adc_read() that returns the value or timeout
      // Wait until ADC ready, with timeout
      while (adc_state != ADC_STATE_DATA) {
        if (adc_timeout++ == 10) {
          break;
        }
        timer_sleep_ms(1, LPM1_bits);
      }

      if (adc_state == ADC_STATE_DATA) {
        adcbatt = adc_result;
      }

      adc_shutdown();
    }
    #endif

    #if RB_USE_RF
    rf_wait_for_idle();
    send_message(adcbatt, temp, blinks);
    rf_shutdown();
    SetVCore(0);
    #endif

  }
}


/*
 * Construct a message and send it over the RF
 */
static void send_message(uint16_t adcbatt, uint16_t rawtemp, uint32_t blinks)
{
  static uint32_t tx_count = 0;
  unsigned char buf[UART_BUF_LEN];
  unsigned char len = 0;
  uint8_t rf_timeout = 0;

  // Append tx counter
  buf[len++] = 'P';
  buf[len++] = ':';
  buf[len++] = ' ';
  len += sc_itoa(tx_count, &buf[len], UART_BUF_LEN - len);
  buf[len++] = ' ';

  // Append battery level
  len += sc_itoa(adcbatt, &buf[len], UART_BUF_LEN - len);
  buf[len++] = ' ';

  // Append temperature
  {
    int16_t temp;
    uint16_t temp_int;
    uint32_t temp_frac;
    char sign = 1;

    temp = (int16_t)rawtemp;

    /* Grab the sign and convert to positive
     * Negative integer math is... interesting so we take the risk of
     * asymmetry and convert to positive. This might incur heavy error
     * for some values, but is verified to work correctly with a fixed
     * dataset in the range of 127.93...-55.00 (from datasheets).
     */
    if (temp < 0) {
      sign = -1;
      temp *= -1;
    }
    /* We always mark the sign to make parsing easier */
    buf[len++] = (sign == 1) ? '+' : '-';

    /* Integer part */
    temp_int = temp >> 8;

    len += sc_itoa(temp_int, buf + len, UART_BUF_LEN - len);

    /* Decimal part is in the low 8 bits, but only top 4 bits are used.
     * One step is roughly 0.06253 degrees Celsius, but no floats here!
     * So we multiply by 6253 and divide by 1000 to catch the first two
     * decimal places
     */
    temp_frac = (((((uint32_t)temp) & 0xf0) >> 4) * 6253) / 1000;
    buf[len++] = '.';

    /* Add leading zero, if needed */
    if (temp_frac < 10) {
      buf[len++] = '0';
    }

    len += sc_itoa(temp_frac, buf + len, UART_BUF_LEN - len);

    buf[len++] = ' ';
  }

  // Append blink count
  len += sc_itoa((int32_t)blinks, &buf[len], UART_BUF_LEN - len);

  // Append newline
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

  ++tx_count;
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

