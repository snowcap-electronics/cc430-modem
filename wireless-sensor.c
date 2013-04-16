/*
 * Wireless temperature sensor
 *
 * Copyright 2013 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

static void send_message(uint16_t batt, uint16_t temp);

#define RB_USE_RF                1
#define RB_USE_ADC               1
#define RB_USE_I2C               1
#define RB_USE_SHUTDOWN_TMP275   0

int main(void)
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  // Select VLOCLK for aux clock source
  UCSCTL4 |= SELA__VLOCLK;

  // Turn off SMCLK (it turns on automatically, if a module uses it)
  // FIXME: Nothing is received, if SMCLK is turned off
  //UCSCTL6 |= SMCLKOFF;

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

  led_init();

  #if SC_USE_SLEEP == 0
  // Enable interrupts
  __bis_status_register(GIE);
  #endif

  // Main loop:
  // - init i2c
  // - initiate tmp275 (will take 220ms) in one shot mode
  // - sleep 220
  // - initiate battery adc
  // - start radio
  // - wait until radio idle
  // - battery adc ready
  // - shutdown adc
  // - read tmp275
  // - shutdown i2c
  // - wait for empty air
  // - send message
  // - shutdown radio
  // - LPM4
  // - sleep minutes
  while(1) {
    uint16_t adcbatt = 0;
    uint16_t temp = 0;

    // Increase PMMCOREV level to 2 for proper radio operation
    SetVCore(2);

    led_on(1);

    #if RB_USE_I2C
    i2c_init();
    #if RB_USE_SHUTDOWN_TMP275
    tmp275_shutdown();

    // FIXME: why i2c_shutdown increased the power consumption by 1-2mA?
    //i2c_shutdown();
    #else
    tmp275_start_oneshot();
    #endif
    #endif

    timer_sleep_ms(220, LPM4_bits);
    led_off(2);

    #if RB_USE_ADC
    adc_start(ADC_CHANNEL_BATTERY, ADC12SHT0_6);
    #endif

    #if RB_USE_RF
    rf_init();

    rf_wait_for_idle();

    rf_receive_on();
    #endif

    #if RB_USE_ADC
    if (1) {
      uint8_t adc_timeout = 0;

      // FIXME: implement adc_read() that return's the value or timeout
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

    #if RB_USE_I2C
    #if RB_USE_SHUTDOWN_TMP275
    // Do nothing if not using TMP275
    #else
    // Read temperature from TMP275
    // FIXME: implement tmp275_read_temp() instead
    temp = i2c_read();

    tmp275_shutdown();

    // FIXME: why i2c_shutdown increased the power consumption by 1-2mA?
    //i2c_shutdown();
    #endif
    #endif

    #if RB_USE_RF
    send_message(adcbatt, temp);
    #endif

    led_on(2);

    #if RB_USE_RF
    rf_shutdown();
    #endif

    led_off(1);
    led_off(2);

    SetVCore(0);

    timer_sleep_ms(4*1000, LPM4_bits);

    //timer_sleep_min(10, LPM4_bits);
  }
}


/*
 * Construct a message and send it over the RF
 */
static void send_message(uint16_t adcbatt, uint16_t rawtemp)
{
  unsigned char buf[UART_BUF_LEN];
  unsigned char len = 0;
  uint8_t rf_timeout = 0;

  // Send battery level
  buf[len++] = 'B';
  buf[len++] = ':';
  buf[len++] = ' ';
  len += sc_itoa(adcbatt, &buf[len], UART_BUF_LEN - len);
  buf[len++] = ',';
  buf[len++] = ' ';

  // Send temperature
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
    buf[len++] = 'T';
    buf[len++] = ':';
    buf[len++] = ' ';

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
    buf[len++] = '\r';
    buf[len++] = '\n';
  }

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

