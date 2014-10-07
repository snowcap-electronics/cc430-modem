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

#ifndef RB_ADC_H
#define RB_ADC_H

#include "common.h"
#include <msp430.h>
#include <stdint.h>

#define ADC_CHANNEL_0           ADC12INCH_0
#define ADC_CHANNEL_1           ADC12INCH_1
#define ADC_CHANNEL_2           ADC12INCH_2
#define ADC_CHANNEL_3           ADC12INCH_3
#define ADC_CHANNEL_BATTERY     ADC12INCH_11 // (AVCC - AVSS) / 2

typedef enum adc_mode_t {
  ADC_MODE_SINGLE,
  ADC_MODE_CONT
} adc_mode_t;

typedef enum adc_state_t {
  ADC_STATE_IDLE = 0,
  ADC_STATE_START_PENDING,
  ADC_STATE_MEASURING,
  ADC_STATE_DATA
} adc_state_t;

extern volatile adc_state_t adc_state;

void adc_start(uint8_t ch_count, uint8_t *chan, unsigned int clks, adc_mode_t mode);
void adc_get_data(uint8_t ch, uint16_t *data, uint32_t *counter);
void adc_shutdown(void);

#endif
