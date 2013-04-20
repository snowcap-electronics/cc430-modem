/*
 * FPS heurestics based on light sensor
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

#ifndef RB_FPS_H
#define RB_FPS_H

#include "common.h"

#include <stdint.h>

uint8_t handle_adc(uint16_t adc, uint32_t timestamp_ms, uint8_t *fps, uint16_t *low, uint16_t *low_limit, uint16_t *high_limit, uint16_t *high);
uint8_t create_message(uint8_t *buf,
                       uint8_t max_len,
                       uint32_t timestamp_ms,
                       uint8_t fps,
                       uint16_t missed_adc,
                       uint16_t frame_len,
                       uint16_t low,
                       uint16_t low_limit,
                       uint16_t high_limit,
                       uint16_t high);

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

