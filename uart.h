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

#ifndef RB_UART_H
#define RB_UART_H

#include "common.h"
#include "rf.h"

#include <msp430.h>

#define UART_BUF_LEN       (PAYLOAD_LEN * 3)   // Bigger buffers for uart

#define UART_RX_NEWDATA_TIMEOUT_MS       4   // 4ms timeout for sending current uart rx data
//#define UART_RX_NEWDATA_TIMEOUT_MS       511   // 511ms timeout for sending current uart rx data

// Buffer for incoming data from UART
extern unsigned char UartRxBuffer[UART_BUF_LEN];
extern unsigned char UartRxBuffer_i;

// Buffer for outoing data over UART
extern unsigned char UartTxBuffer[UART_BUF_LEN];
extern unsigned char UartTxBuffer_i;
extern unsigned char UartTxBufferLength;
extern unsigned char uart_rx_timeout;


void uart_init(void);
int handle_uart_rx_byte(void);

#endif
