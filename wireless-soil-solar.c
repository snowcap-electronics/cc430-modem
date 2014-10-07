/*
 * Wireless solar powered soil moisture sensor 
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

#define DEBUG_MODE 0

// FIXME: these probably will change per temperature?
#define SUPER_CAP_LOW_LIMIT               1000
#define SUPER_CAP_FULL_LIMIT              2200
#define SOLAR_PANEL_MAX_LIMIT             2500

static uint8_t ADC_CHANNELS[] = {
  ADC_CHANNEL_0,
  ADC_CHANNEL_1,
  ADC_CHANNEL_2,
  ADC_CHANNEL_3,
  ADC_CHANNEL_BATTERY
};

typedef enum power_flags_t {
  POWER_FLAG_SUPERCAP_LOW   = 0x01,
  POWER_FLAG_SUPERCAP_FULL  = 0x02,
  POWER_FLAG_SOLAR_MAX      = 0x04
} power_flags_t;

#define ADC_PINS                 (BIT0 | BIT1 | BIT2 | BIT3)

static void send_message(uint32_t *adc, uint16_t temp);
static void get_adc(uint32_t adcdata[], uint8_t min_ch, uint8_t max_ch);

#define RB_USE_I2C               1
#define RB_USE_SHUTDOWN_TMP275   0

static uint8_t power_state;

int main(void)
{
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

  #if DEBUG_MODE
  led_init();
  #endif

  #if SC_USE_SLEEP == 0
  // Enable interrupts
  __bis_status_register(GIE);
  #endif
  
  #if RB_USE_I2C
  // Shutdown TMP275 to save power
  i2c_init();
  tmp275_shutdown();
  // FIXME: why i2c_shutdown increased the power consumption by 1-2mA?
  // FIXME: because of pull ups?
  //i2c_shutdown();
  #endif

  while(1) {
    uint32_t adcdata[sizeof(ADC_CHANNELS)] = {0};
    uint16_t temp = 0;
    uint8_t sleep_min = 60;

    // Initialise power state
    power_state = 0;

    #if RB_USE_I2C
    tmp275_start_oneshot();
    timer_sleep_ms(220, LPM4_bits);
    temp = i2c_read();
    // TMP275 will shutdown after one shot conversion
    #endif

    // Initialise analog pins
    P2SEL |= ADC_PINS;
    P2DIR &= ~ADC_PINS;

    // Read solar harvester state
    get_adc(adcdata, 2, 4);

    // Check for power states
    // FIXME: bad limits at least with 2.5V VDD, ignoring for now
    if (0){
    if (adcdata[3] < SUPER_CAP_LOW_LIMIT) {
      power_state |= POWER_FLAG_SUPERCAP_LOW;
    }
    if (adcdata[3] > SUPER_CAP_FULL_LIMIT) {
      power_state |= POWER_FLAG_SUPERCAP_FULL;
    }
    if (adcdata[2] > SOLAR_PANEL_MAX_LIMIT) {
      power_state |= POWER_FLAG_SOLAR_MAX;
    }
    }

    // If supercap voltage too low, skip moisture measurement and radio usage
    if ((power_state & POWER_FLAG_SUPERCAP_LOW) == 0) {

      // Configure GD2 (P1.1)
      PMAPPWD = 0x02D52;              // Get write-access to port mapping regs
      P1MAP1  = PM_RFGDO2;            // Map RFGDO2 output to P1.1
      PMAPPWD = 0;                    // Lock port mapping registers
      P1SEL   |= BIT1;

      // Increase PMMCOREV level to 2 for proper radio operation
      SetVCore(2);
      rf_init();

      // gdo2 output configuration,
      // 0x39 == RFCLK/24 (1.083MHz)
      // 0x38 == RFCLK/16 (1.625MHz)
      // 0x34 == RFCLK/4  (6.5MHz)
      // 0x32 == RFCLK/2  (13MHz)
      // 0x30 == RFCLK/1  (26MHz)
      WriteSingleReg(IOCFG2, 0x39);

      // Enable TEMP_ADJ_VCC
      P1OUT |= BIT4;

      // Wait for the moisture sensor to stabilize
      busysleep_ms(250);

      // Read soil moisture and reference
      get_adc(adcdata, 0, 1);

      // Disable TEMP_ADJ_VCC
      P1OUT &= ~BIT4;

      // gdo2 output configuration, 0x29 == RF_RDY
      WriteSingleReg(IOCFG2, 0x29);

      rf_wait_for_idle();
      send_message(adcdata, temp);
      rf_shutdown();
      SetVCore(0);

      // De-configure GD2 (P1.1)
      PMAPPWD = 0x02D52;              // Get write-access to port mapping regs
      P1MAP1  = 0x0;                  // Map GPIO to P1.1
      PMAPPWD = 0;                    // Lock port mapping registers
      P1SEL  &= ~BIT1;
    }

    // Analog pins to digital output low
    P2DIR |= ADC_PINS;
    P2SEL &= ~ADC_PINS;
    P2OUT &= ~ADC_PINS;

    // Sleep between measurements
    #if DEBUG_MODE
    {
      uint8_t i;
      for (i = 0; i < 5; i++) {
        // LED 2 conflicts with TEMP_ADJ_VCC (PM1.4)
        led_toggle(1);
        timer_sleep_ms(100, LPM4_bits);
      }
    }
    #else
    if (power_state & (POWER_FLAG_SUPERCAP_FULL | POWER_FLAG_SUPERCAP_FULL)) {
      // If full power, sleep only 10 minutes
      sleep_min = 10;
    } else if (power_state & POWER_FLAG_SUPERCAP_LOW) {
      // Low on power, sleep 2 hours
      sleep_min = 120;
    }
    timer_sleep_min(sleep_min, LPM4_bits);
    #endif

  }
}


/*
 * Construct a message and send it over the RF
 */
static void send_message(uint32_t *adc, uint16_t rawtemp)
{
  static uint32_t tx_count = 0;
  unsigned char buf[UART_BUF_LEN];
  unsigned char len = 0;
  uint8_t i;

  // Append tx counter
  buf[len++] = 'S';
  buf[len++] = ':';
  buf[len++] = ' ';
  len += sc_itoa(tx_count, &buf[len], UART_BUF_LEN - len);
  buf[len++] = ' ';

  // Append ADC values
  for (i = 0; i < sizeof(ADC_CHANNELS); ++i) {
    len += sc_itoa(adc[i], &buf[len], UART_BUF_LEN - len);
    buf[len++] = ' ';
  }

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
  }

  // Append newline
  buf[len++] = '\r';
  buf[len++] = '\n';

  // Send the message
#if 1
  {
    uint8_t rf_timeout = 0;

    rf_append_msg(buf, len);
    rf_send_next_msg(RF_SEND_MSG_FORCE);

    // Wait for completion of the tx, with timeout
    while (rf_transmitting) {
      if (rf_timeout++ == 200) {
        break;
      }
      busysleep_us(100);
    }
  }
#else
  uart_tx_append_msg(buf, len);
  uart_send_next_msg();
#endif

  ++tx_count;
}

/*
 * Fill in channels from min_ch to max_ch in adcdata
 */
static void get_adc(uint32_t adcdata[], uint8_t min_ch, uint8_t max_ch)
{
  uint8_t adc_count = 0;
  uint8_t j = 0;
  uint8_t ch;
  uint8_t ch_count = 0;
  uint8_t adc_channels[sizeof(ADC_CHANNELS)];

  // Select channels from the list of all
  for (ch = min_ch; ch <= max_ch; ++ch) {
    adc_channels[ch_count++] = ADC_CHANNELS[ch];
  }

  adc_start(ch_count, adc_channels, ADC12SHT1_12, ADC_MODE_CONT);

  do {
    uint16_t adc_timeout = 0;
    // FIXME: implement adc_read() that returns the value or timeout
    // Wait until ADC ready, with timeout
    while (adc_state != ADC_STATE_DATA) {
      if (adc_timeout++ == 10000) {
        break;
      }
      busysleep_us(100);
    }

    if (adc_state == ADC_STATE_DATA) {
      uint16_t data;
      
      for (j = 0; j < ch_count; ++j) {
        adc_get_data(j, &data, (void*)0);
        adcdata[j + min_ch] += data;
      }
      ++adc_count;
    }
  } while (adc_count < 16); // 16 times, i.e. 4 extra bits

  adc_shutdown();

  // Calculate the ADC average
  for (j = 0; j < ch_count; ++j) {
    adcdata[j + min_ch] >>= 4;
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

