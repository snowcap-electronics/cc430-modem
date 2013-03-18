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

#ifndef RB_RF_H
#define RB_RF_H

#include "common.h"

#include "RF1A.h"
#include "hal_pmm.h"

#include <msp430.h>
#include <stdint.h>

#define PAYLOAD_LEN        (60)                // Max payload
#define PACKET_LEN         (PAYLOAD_LEN + 3)   // PACKET_LEN = payload + len + RSSI + LQI
#define RF_QUEUE_LEN       (PAYLOAD_LEN * 3)   // Space for several messages
#define CRC_OK             (BIT7)              // CRC_OK bit
#define PATABLE_VAL        (0xC3)              // +10 dBm output
//#define PATABLE_VAL        (0x51)              // 0 dBm output


#define CC430_STATE_TX                   (0x20)
#define CC430_STATE_IDLE                 (0x00)
#define CC430_STATE_TX_UNDERFLOW         (0x70)
#define CC430_STATE_MASK                 (0x70)
#define CC430_FIFO_BYTES_AVAILABLE_MASK  (0x0F)
#define CC430_STATE_RX                   (0x10)
#define CC430_STATE_RX_OVERFLOW          (0x60)

// Buffer for incoming data from RF
extern unsigned char RfRxBuffer[PACKET_LEN];
extern unsigned char RfRxBufferLength;

// Queue for messages to be sent out over RF
extern unsigned char RfTxQueue[RF_QUEUE_LEN];
extern unsigned char RfTxQueue_i;

// Buffer for message currently being sent out over RF
extern unsigned char RfTxBuffer[PACKET_LEN];
extern unsigned char rf_error;

extern unsigned char rf_transmitting;
extern unsigned char rf_receiving;


void rf_init(void);
void rf_shutdown(void);
void rf_receive_on(void);
void rf_receive_off(void);
void rf_append_msg(unsigned char *buf, unsigned char len);
void rf_send_next_msg(uint8_t force);

#endif
