/*
 * UART handling
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

#include "uart.h"

// Buffer for incoming data from UART
unsigned char UartRxBuffer[UART_BUF_LEN];
unsigned char UartRxBuffer_i = 0;

// Buffer for outoing data over UART
unsigned char UartTxBuffer[UART_BUF_LEN];
unsigned char UartTxBuffer_i = 0;
unsigned char UartTxBufferLength = 0;
unsigned char uart_rx_timeout = 0;

static void handle_uart_rx_byte(void);

/*
 * Map P1.5 & P1.6 to Uart TX and RX and initialise Uart as 115200 8N1
 * with interrupts enabled
 */
void uart_init(void)
{
  PMAPPWD = 0x02D52;                        // Get write-access to port mapping regs
  P1MAP5 = PM_UCA0RXD;                      // Map UCA0RXD output to P1.6
  P1MAP6 = PM_UCA0TXD;                      // Map UCA0TXD output to P1.5
  PMAPPWD = 0;                              // Lock port mapping registers

  P1DIR |= BIT6;                            // Set P1.6 as TX output
  P1SEL |= BIT5 + BIT6;                     // Select P1.5 & P1.6 to UART function

  UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA0CTL1 |= UCSSEL_2;                     // SMCLK
  UCA0BR0 = 9;                              // 1MHz 115200 (see User's Guide)
  UCA0BR1 = 0;                              // 1MHz 115200
  UCA0MCTL |= UCBRS_1 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  UCA0IE |= UCTXIE;                         // Enable USCI_A0 TX interrupt
}



/*
 * Uart TX or RX ready (one byte)
 */
__attribute__((interrupt(USCI_A0_VECTOR)))
void USCI_A0_ISR(void)
{

  switch(UCA0IV) {
  case 0: break;                            // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    handle_uart_rx_byte();
#if SC_USE_SLEEP == 1
    // Exit active
    __bic_status_register_on_exit(LPM3_bits);
#endif
    break;
  case 4:                                   // Vector 4 - TXIFG
    if (UartTxBufferLength == 0) {          // Spurious interrupt (or a workaround for a bug)?
      return;
    }

    if (++UartTxBuffer_i == UartTxBufferLength) { // All data sent?
      UartTxBuffer_i = 0;                   // Clear the Uart TX buffers
      UartTxBufferLength = 0;
      return;
    }

    // More data to be sent to Uart
    //while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready? (should be always in here??)
    UCA0TXBUF = UartTxBuffer[UartTxBuffer_i]; // Send a byte

    break;
  default: break;
  }
}



/*
 * Called from interrupt handler to handle the received byte over uart
 */
static void handle_uart_rx_byte(void)
{
  unsigned char tmpchar;

  tmpchar = UCA0RXBUF;

  // Discard the byte if buffer already full
  if (UartRxBuffer_i == UART_BUF_LEN) {
    return;
  }

  // Store received byte
  UartRxBuffer[UartRxBuffer_i++] = tmpchar;

  return;
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
