/*
 * Generic I2C commands
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

#include "i2c.h"

#define RB_I2C_MAX_RX_BYTES  8

volatile enum i2c_state_t i2c_rx_state;
volatile enum i2c_state_t i2c_tx_state;

static volatile unsigned char I2CTXByteCtr;                  // Bytes to send
static volatile unsigned char I2CRXByteCtr;                  // Bytes to receive
static volatile unsigned char *I2CPTxData;                   // Pointer to TX data
static volatile unsigned char *I2CPRxData;          // Pointer to RX data
static volatile unsigned char I2CRxBuffer[RB_I2C_MAX_RX_BYTES];       // I2C RX buffer

/*
 * I2C ISR
 */
__attribute__((interrupt(USCI_B0_VECTOR)))
void USCI_B0_ISR(void)
{
  switch(UCB0IV) {
  case  0: break;                           // Vector  0: No interrupts
  case  2: break;                           // Vector  2: ALIFG
  case  4: break;                           // Vector  4: NACKIFG
  case  6: break;                           // Vector  6: start condition
  case  8: break;                           // Vector  8: stop condition
  case 10:                                  // Vector 10: RXIFG
    I2CRXByteCtr--;                         // Decrement RX byte counter
    *I2CPRxData++ = UCB0RXBUF;              // Move RX data to address PRxData
    if (I2CRXByteCtr > 0) {
      if (I2CRXByteCtr == 1) {              // Only one byte left?
        UCB0CTL1 |= UCTXSTP;                // Generate I2C stop condition
      }
    } else {
      i2c_rx_state = I2C_IDLE;
#if SC_USE_SLEEP == 1
      __bic_status_register_on_exit(LPM3_bits); // Exit LPMx
#endif
    }
    break;
  case 12:                                  // Vector 12: TXIFG
    if (I2CTXByteCtr) {                     // Check TX byte counter
      UCB0TXBUF = *I2CPTxData++;            // Load TX buffer
      I2CTXByteCtr--;                       // Decrement TX byte counter
    } else {
      UCB0CTL1 |= UCTXSTP;                  // I2C stop condition
      UCB0IFG &= ~UCTXIFG;                  // Clear USCI_B0 TX int flag
      i2c_tx_state = I2C_IDLE;
#if SC_USE_SLEEP == 1
      __bic_status_register_on_exit(LPM3_bits); // Exit LPMx
#endif
    }
    break;
  default: break;
  }
}



/*
 * Initialize I2C.
 */
void i2c_init(void)
{
  i2c_rx_state = I2C_IDLE;
  i2c_tx_state = I2C_IDLE;

  PMAPPWD = 0x02D52;                        // Get write-access to port mapping regs
  P1MAP3 = PM_UCB0SDA;                      // Map UCB0SDA output to P1.3
  P1MAP2 = PM_UCB0SCL;                      // Map UCB0SCL output to P1.2
  PMAPPWD = 0;                              // Lock port mapping registers

  P1SEL |= BIT2 + BIT3;                     // Select P1.2 & P1.3 to I2C function

  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
  UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
  UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
  UCB0BR1 = 0;
  // FIXME: shouldn't set slave address here? At least a parameter
  UCB0I2CSA = 0x4F;                         // TMP275 Slave Address is 04Fh
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  UCB0IE |= UCTXIE + UCRXIE;                // Enable interrupts
}



/*
 * I2C send n bytes synchronously
 */
void i2c_send(unsigned char *buf, unsigned char bytes)
{
  I2CTXByteCtr = bytes;                     // Load TX byte counter
  I2CPTxData = buf;                         // TX array start address

  i2c_tx_state = I2C_ACTIVE;

  while (UCB0CTL1 & UCTXSTP);               // Ensure stop condition got sent
  UCB0CTL1 |= UCTR + UCTXSTT;               // I2C TX, start condition

  while (i2c_tx_state == I2C_ACTIVE) {
#if SC_USE_SLEEP == 1
    __bis_status_register(LPM0_bits + GIE); // Enter LPM0, enable interrupts
                                            // Remain in LPM0 until all data
                                            // is TX'd
#endif
  }

  while (UCB0CTL1 & UCTXSTP);               // Ensure stop condition got sent

}



/*
 * I2C read 2 bytes synchronously
 */
uint16_t i2c_read(void)
{
  I2CPRxData = I2CRxBuffer;                 // Start of RX buffer
  I2CRXByteCtr = 2;                         // Load RX byte counter

  i2c_rx_state = I2C_ACTIVE;

  while (UCB0CTL1 & UCTXSTP);               // Ensure stop condition got sent
  UCB0CTL1 &= ~UCTR;                        // Make sure TX is not set
  UCB0CTL1 |= UCTXSTT;                      // I2C start condition

#if 0
  // FIXME: if receiving only 1 byte, need to send stop immediately
  while(UCB0CTL1 & UCTXSTT);                // Start condition sent?
  UCB0CTL1 |= UCTXSTP;                      // I2C stop condition
#endif

  while (i2c_rx_state == I2C_ACTIVE) {
#if SC_USE_SLEEP == 1
    __bis_status_register(LPM0_bits + GIE); // Enter LPM0, enable interrupts
                                            // Remain in LPM0 until all data
                                            // is RX'd
#endif
  }

  return ((I2CRxBuffer[0] << 8) | I2CRxBuffer[1]);
}


/*
 * Shutdown I2C
 */
void i2c_shutdown(void)
{
  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  P1SEL &= ~(BIT2 + BIT3);                  // Unselect P1.2 & P1.3 to I2C function
  P1OUT &= ~(BIT2 + BIT3);                  // Set low
  P1DIR |= BIT2 + BIT3;                     // Set direction out
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

