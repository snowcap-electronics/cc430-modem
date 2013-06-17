/*
 * RF commands
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

#include "rf.h"
#include "uart.h"
#include "led.h"
#include "utils.h"
#include "timer.h"

// Buffer for incoming data from RF
volatile unsigned char RfRxBuffer[PACKET_LEN];
volatile unsigned char RfRxBufferLength = 0;

// Queue for messages to be sent out over RF
volatile unsigned char RfTxQueue[RF_QUEUE_LEN];
volatile unsigned char RfTxQueue_i = 0;

// Buffer for outgoing data over RF
volatile unsigned char RfTxBuffer[PACKET_LEN];
volatile unsigned char rf_error = 0;

volatile unsigned char rf_transmitting = 0;
volatile unsigned char rf_receiving = 0;

static void transmit_msg(unsigned char *buffer, unsigned char length);
static void handle_rf_rx_packet(void);

/*
 * Initialize CC1101 radio inside the CC430.
 */
void rf_init(void)
{
#if 0
  // Set the High-Power Mode Request Enable bit so LPM3 can be entered
  // with active radio enabled
  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMHPMRE_L;
  PMMCTL0_H = 0x00;
#endif

  Strobe(RF_SRES);                          // Reset the Radio Core
  Strobe(RF_SNOP);                          // Reset Radio Pointer

  RfRxBufferLength = 0;
  RfTxQueue_i = 0;
  rf_error = 0;
  rf_transmitting = 0;
  rf_receiving = 0;

  WriteRfSettings();

  WriteSinglePATable(PATABLE_VAL);
}



/*
 * Wait until radio is idle
 */
void rf_wait_for_idle(void)
{
  unsigned char RxStatus;

  while (((RxStatus = Strobe(RF_SNOP)) & CC430_STATE_MASK) != CC430_STATE_IDLE) {
    timer_sleep_ms(1, LPM1_bits);
  }
}


/*
 * Shutdown CC1101 radio inside the CC430.
 */
void rf_shutdown(void)
{
  Strobe(RF_SIDLE);
  Strobe(RF_SPWD);
}



/*
 * Turn on RF receiver
 */
void rf_receive_on(void)
{
  rf_receiving = 1;

  // Falling edge of RFIFG9
  RF1AIES |= BIT9;

  // Clear a pending interrupt
  RF1AIFG &= ~BIT9;

  // Enable the interrupt
  RF1AIE  |= BIT9;

  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  Strobe(RF_SRX);
}



/*
 * Turn off RF receiver
 */
void rf_receive_off(void)
{

  // Disable RX interrupts
  RF1AIE &= ~BIT9;

  // Clear pending IFG
  RF1AIFG &= ~BIT9;

  // It is possible that ReceiveOff is called while radio is receiving a packet.
  // Therefore, it is necessary to flush the RX FIFO after issuing IDLE strobe
  // such that the RXFIFO is empty prior to receiving a packet.
  Strobe(RF_SIDLE);
  Strobe(RF_SFRX);

  rf_receiving = 0;
}



/*
 * RF TX or RX ready (one whole message)
 */
__attribute__((interrupt(CC1101_VECTOR)))
void CC1101_ISR(void)
{
  switch(RF1AIV) {                          // Prioritizing Radio Core Interrupt
  case  0: break;                           // No RF core interrupt pending
  case  2: break;                           // RFIFG0
  case  4: break;                           // RFIFG1
  case  6: break;                           // RFIFG2
  case  8: break;                           // RFIFG3
  case 10: break;                           // RFIFG4
  case 12: break;                           // RFIFG5
  case 14: break;                           // RFIFG6
  case 16: break;                           // RFIFG7
  case 18: break;                           // RFIFG8
  case 20:                                  // RFIFG9

    // Disable RFIFG9 interrupts
    RF1AIE &= ~BIT9;

    // RX end of packet
    if(rf_receiving) {
      handle_rf_rx_packet();
    }

    // RF TX end of packet
    if(rf_transmitting) {
      rf_transmitting = 0;
    }
    break;
    // FIXME: RFIFG10 == RX with valid CRC?
  case 22: break;                           // RFIFG10
  case 24: break;                           // RFIFG11
  case 26: break;                           // RFIFG12
  case 28: break;                           // RFIFG13
  case 30: break;                           // RFIFG14
  case 32: break;                           // RFIFG15
  }

#if SC_USE_SLEEP == 1
  __bic_status_register_on_exit(LPM0_bits); // Exit active
#endif
}



/*
 * Append new message to transmit queue
 */
void rf_append_msg(unsigned char *buf, unsigned char len)
{
  unsigned char i;

  // Disable interrupts to make sure RfTxQueue isn't modified in the middle
  __bic_status_register(GIE);

  // Discard msg if no space in the queue
  if ((RfTxQueue_i + len) > RF_QUEUE_LEN) {
    // Enable interrupts
    __bis_status_register(GIE);
    return;
  }

  for (i = 0; i < len; ++i) {
    RfTxQueue[RfTxQueue_i + i] = buf[i];
  }

  RfTxQueue_i += len;

  // Enable interrupts
  __bis_status_register(GIE);
}



/*
 * Send next message from the queue
 */
uint8_t rf_send_next_msg(enum RF_SEND_MSG force)
{
  unsigned char x;
  int len = -1;

  // Do nothing, if already transmitting
  if (rf_transmitting) {
    return 0;
  }

  // Disable interrupts to make sure RfTxQueue isn't modified in the middle
  __bic_status_register(GIE);

  if (force) {
    len = RfTxQueue_i;
  } else {
    // Find the end of the first message
    for (x = 0; x < RfTxQueue_i; x++) {
      if (RfTxQueue[x] == '\n') {
        len = x + 1;
        break;
      }
    }

    // No newline, do nothing
    if (len == -1) {
      // Enable interrupts
      __bis_status_register(GIE);
      return 0;
    }
  }

  // Radio expects first byte to be packet len (excluding the len byte itself)
  RfTxBuffer[0] = len;
  // Copy data received over uart to RF TX buffer
  for (x = 0; x < len; ++x) {
    RfTxBuffer[x+1] = RfTxQueue[x];
  }

  // If we have more data to send, move them in front of the buffer
  if (len != RfTxQueue_i) {
    for (x = 0; x < RfTxQueue_i - len; ++x) {
      RfTxQueue[x] = RfTxQueue[x+len];
    }
    RfTxQueue_i -= len;
  } else {
    // All data sent, clear uart buffers
    RfTxQueue_i = 0;
  }

  // Stop receive mode
  if (rf_receiving) {
    rf_receive_off();
  }

  // Send buffer over RF (len +1 for length byte)
  rf_transmitting = 1;
  len = RfTxBuffer[0];
  transmit_msg((unsigned char*)RfTxBuffer, len + 1);

  // Enable interrupts
  __bis_status_register(GIE);

  return len;
}



/*
 * Called from interrupt handler to handle a received packet from radio
 */
static void handle_rf_rx_packet(void)
{
  unsigned char x;
  unsigned char RxStatus;

  // Radio is in IDLE after receiving a message (See MCSM0 default values)
  rf_receiving = 0;

  // Validate radio state
  RxStatus = Strobe(RF_SNOP);
  if ((RxStatus & CC430_STATE_MASK) != CC430_STATE_IDLE) {
    rf_error = 1;
    goto rx_error;
  }

  // Read the length byte from the FIFO
  RfRxBufferLength = ReadSingleReg(RXBYTES);

  // Must have at least 4 bytes (len <payload> RSSI CRC) for a valid packet
  if (RfRxBufferLength < 4) {
    goto rx_error;
  }

  // Read the packet data
  ReadBurstReg(RF_RXFIFORD, (unsigned char *)RfRxBuffer, RfRxBufferLength);

  // Verify CRC
  if(!(RfRxBuffer[RfRxBufferLength - 1] & CRC_OK)) {
    goto rx_error;
  }

  // FIXME: use uart API instead of directly poking the buffers

  // If there's not enough space for new data in uart tx buffer, discard new data
  if (UartTxBufferLength + (RfRxBufferLength - 3) > UART_BUF_LEN) {
    goto failed_to_receive;
  }

  // Append the RF RX buffer to Uart TX, skipping the length, RSSI and CRC/Quality bytes
  for (x = 0; x < RfRxBufferLength - 3; ++x) {
    UartTxBuffer[UartTxBufferLength + x] = RfRxBuffer[x+1];
  }
  UartTxBufferLength += (RfRxBufferLength - 3);

  {
    // DEBUG: Write RSSI to UartTxBuffer
    // Remove \r\n
    UartTxBufferLength -= 2;
    UartTxBuffer[UartTxBufferLength++] = ' ';
    unsigned char len;
    volatile unsigned char *buf = &UartTxBuffer[UartTxBufferLength];
    unsigned char value = RfRxBuffer[RfRxBufferLength - 2];
    unsigned char max_len = UART_BUF_LEN - UartTxBufferLength;
    int16_t rssi;

    // Convert RSSI to 0-255, 255 being the best signal
    if (value >= 128) {
      rssi = value - 256;
    } else {
      rssi = value;
    }
    rssi -= 2*74; // double RSSI offset from data sheet

    // turn negative value to 0-255, 255 being the best signal
    rssi += 276;

    len = sc_itoa(rssi, (unsigned char *)buf, max_len);
    if (len == 0) {
      UartTxBuffer[UartTxBufferLength] = 'X';
      ++len;
    }
    UartTxBuffer[UartTxBufferLength + len++] = ' ';
    UartTxBufferLength += len;
  }

  {
    // DEBUG: Write CRC/LQI to UartTxBuffer
    unsigned char len;
    volatile unsigned char *buf = &UartTxBuffer[UartTxBufferLength];
    unsigned char value = RfRxBuffer[RfRxBufferLength - 1];
    unsigned char max_len = UART_BUF_LEN - UartTxBufferLength;
    len = sc_itoa(value, (unsigned char *)buf, max_len);
    if (len == 0) {
      UartTxBuffer[UartTxBufferLength] = 'X';
      ++len;
    }
    UartTxBuffer[UartTxBufferLength + len++] = '\r';
    UartTxBuffer[UartTxBufferLength + len++] = '\n';
    UartTxBufferLength += len;
  }

  // Send the first byte to Uart
  //while (!(UCA0IFG&UCTXIFG));           // USCI_A0 TX buffer ready?
  UCA0TXBUF = UartTxBuffer[UartTxBuffer_i]; // Send first byte
  return;

 rx_error:
  rf_error = 1;

 failed_to_receive:
  RfRxBufferLength = 0;
  return;
}




/*
 * Start RF transmit with the given message
 */
static void transmit_msg(unsigned char *buffer, unsigned char length)
{
  // Falling edge of RFIFG9
  RF1AIES |= BIT9;

  // Clear pending interrupts
  RF1AIFG &= ~BIT9;

  // Enable TX end-of-packet interrupt
  RF1AIE |= BIT9;

  WriteBurstReg(RF_TXFIFOWR, buffer, length);

  // Start transmit
  Strobe(RF_STX);
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
