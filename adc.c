/*
 * Generic ADC commands
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

#include "adc.h"
#include "led.h"

#define ADC_MAX_CHANNELS                5

volatile adc_state_t adc_state;

static volatile uint16_t adc_result[ADC_MAX_CHANNELS];
static volatile uint32_t adc_counter[ADC_MAX_CHANNELS];
static adc_mode_t adc_mode;

/*
 * ADC interrupt
 */
__attribute__((interrupt(ADC12_VECTOR)))
void ADC12_ISR(void)
{

  switch(ADC12IV) {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case 14:                                  // Vector 14:  ADC12IFG4
    adc_result[4] = ADC12MEM4;
    ++adc_counter[4];
  case 12:                                  // Vector 12:  ADC12IFG3
    adc_result[3] = ADC12MEM3;
    ++adc_counter[3];
  case 10:                                  // Vector 10:  ADC12IFG2
    adc_result[2] = ADC12MEM2;
    ++adc_counter[2];
  case  8:                                  // Vector  8:  ADC12IFG1
    adc_result[1] = ADC12MEM1;
    ++adc_counter[1];
  case  6:                                  // Vector  6:  ADC12IFG0
    adc_result[0] = ADC12MEM0;
    ++adc_counter[0];

    adc_state = ADC_STATE_DATA;

#if SC_USE_SLEEP == 1
    // Exit active
    __bic_status_register_on_exit(LPM3_bits);
#endif
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}



/*
 * Initiate ADC measurement(s)
 */
void adc_start(uint8_t ch_count, uint8_t *chan, unsigned int clks, adc_mode_t mode)
{
  adc_counter[0]  = 0;
  adc_counter[1]  = 0;
  adc_counter[2]  = 0;
  adc_counter[3]  = 0;
  adc_counter[4]  = 0;
  adc_mode        = mode;

  ADC12CTL0  &= ~ADC12ENC;                     // Disable ADC

  // Enable 2.5V shared reference
  REFCTL0 |= REFMSTR + REFVSEL_2 + REFON;

  ADC12CTL0   = clks + ADC12ON;                // Enable ADC with specified sample-and-hold time

  ADC12CTL1   = ADC12SSEL0 /*+ ADC12SSEL1*/ + ADC12SHP;         // Select A/*SM*/CLK

  if (mode == ADC_MODE_CONT || ch_count > 1) {
    ADC12CTL0 |= ADC12MSC;

    if (mode == ADC_MODE_SINGLE) {
      if (ch_count > 1) {
        ADC12CTL1 |= ADC12CONSEQ_1; // sequence-of-channels
      }
    } else {
      if (ch_count == 1) {
        ADC12CTL1 |= ADC12CONSEQ_2; // repeat-single-channel
      } else {
        ADC12CTL1 |= ADC12CONSEQ_3; // repeat-sequence-of-channels
      }
    }
  }

  if (ch_count > 1) {
    // Mark last channel with EOS
    chan[ch_count - 1] |= ADC12EOS;
  }

  switch(ch_count) {
  case 5:
    ADC12MCTL4   = ADC12SREF_1;                   // V(R+) = VREF+ and V(R-) = AVSS
    ADC12MCTL4  |= chan[4];                       // Measure channel
    ADC12IE     |= ADC12IE4;                      // Enable ADC interrupt
  case 4:
    ADC12MCTL3   = ADC12SREF_1;                   // V(R+) = VREF+ and V(R-) = AVSS
    ADC12MCTL3  |= chan[3];                       // Measure channel
    if (ch_count == 4) {
      ADC12IE     |= ADC12IE3;                    // Enable ADC interrupt
    }
  case 3:
    ADC12MCTL2   = ADC12SREF_1;                   // V(R+) = VREF+ and V(R-) = AVSS
    ADC12MCTL2  |= chan[2];                       // Measure channel
    if (ch_count == 3) {
      ADC12IE     |= ADC12IE2;                    // Enable ADC interrupt
    }
  case 2:
    ADC12MCTL1   = ADC12SREF_1;                   // V(R+) = VREF+ and V(R-) = AVSS
    ADC12MCTL1  |= chan[1];                       // Measure channel
    if (ch_count == 2) {
      ADC12IE     |= ADC12IE1;                    // Enable ADC interrupt
    }
  case 1:
    ADC12MCTL0   = ADC12SREF_1;                   // V(R+) = VREF+ and V(R-) = AVSS
    ADC12MCTL0  |= chan[0];                       // Measure channel
    if (ch_count == 1) {
      ADC12IE     |= ADC12IE0;                      // Enable ADC interrupt
    }
    break;
  }

  ADC12CTL2  |= ADC12RES_2;                    // 12bit resolution

  adc_state = ADC_STATE_MEASURING;

  ADC12CTL0 |= ADC12ENC + ADC12SC;             // Enable and start conversion

}


/*
 * Read latest ADC measurement and it's sequence number
 */
void adc_get_data(uint8_t ch, uint16_t *data, uint32_t *counter)
{
    // Disable interrupts to make sure data matches the counter
  __bic_status_register(GIE);

  *data = adc_result[ch];
  if (counter) {
    *counter = adc_counter[ch];
  }

  // Reset the state
  if (adc_mode == ADC_MODE_SINGLE) {
    adc_state = ADC_STATE_IDLE;
  } else {
    adc_state = ADC_STATE_MEASURING;
  }

  // Enable interrupts
  __bis_status_register(GIE);
}

/*
 * Shutdown ADC
 */
void adc_shutdown(void)
{
  ADC12CTL0  = 0;
  ADC12CTL1  = 0;
  ADC12MCTL0 = 0;
  ADC12MCTL1 = 0;
  ADC12MCTL2 = 0;
  ADC12MCTL3 = 0;
  ADC12MCTL4 = 0;
  ADC12IE    = 0;
  ADC12IFG   = 0;
  REFCTL0    = 0;

  adc_state  = ADC_STATE_IDLE;
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

