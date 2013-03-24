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

enum adc_state_t adc_state;
uint16_t         adc_result;


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
  case  6:                                  // Vector  6:  ADC12IFG0
    if (adc_state == ADC_MEASURING) {
      adc_result = ADC12MEM0;
      adc_state = ADC_DATA;

#if SC_USE_SLEEP == 1
      // Exit active
      __bic_status_register_on_exit(LPM3_bits);
#endif
      return;
    }

    break;
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
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
 * Initiate a single ADC measure of the operating voltage
 */
void adc_start(unsigned char chan)
{
  ADC12CTL0  &= ~ADC12ENC;                     // Disable ADC

  // Enable 2.0V shared reference, disable temperature sensor to save power
  REFCTL0 |= REFMSTR + REFVSEL_1 + REFON + REFTCOFF;

  ADC12MCTL0  = ADC12SREF_1;                   // V(R+) = VREF+ and V(R-) = AVSS
  ADC12MCTL0 |= chan;                          // Measure channel

  ADC12CTL2  |= ADC12RES_2;                    // 12bit resolution

  ADC12CTL0   = ADC12SHT0_6 + ADC12ON;         // 128 clks
  ADC12CTL1   = ADC12SSEL1 + ADC12SHP;         // Select ACLK

  ADC12IE     = ADC12IE0;                      // enable ADC interrupt

  adc_state = ADC_MEASURING;

  ADC12CTL0 |= ADC12ENC + ADC12SC;             // Enable and start conversion

}



/*
 * Shutdown ADC to save power
 */
void adc_shutdown(void)
{
  ADC12CTL0 &= ~ADC12ENC;                     // Disable ADC
  ADC12CTL1 |= ADC12TCOFF;                    // Disable temperature sersor to save power
  // FIXME: turn off reference, maybe like this:
  // REFCTL0 &= ~ REFON;
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

