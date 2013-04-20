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

#include "common.h"
#include "fps.h"
#include "utils.h"

#include <stdint.h>

#define FPS_PREV_FRAMES  100
#define FPS_STATE_INIT     0
#define FPS_STATE_HIGH     1
#define FPS_STATE_LOW      2

static uint32_t frames[FPS_PREV_FRAMES] = { 0 };
static uint16_t frame_i = 0;
static uint8_t  state = FPS_STATE_INIT;
static uint16_t high = 0;
static uint16_t low = 0xffff;
static uint16_t new_high = 0;
static uint16_t new_low = 0xffff;

/*
 * Takes brightness and timestamp as parameters and returns FPS (and limits) on a new frame
 */
uint8_t handle_adc(uint16_t adc, uint32_t timestamp_ms, uint8_t *fps, uint16_t *low_ret, uint16_t *low_limit, uint16_t *high_limit, uint16_t *high_ret)
{

  uint16_t quarter_range;
  uint16_t i;
  uint8_t is_new_frame = 0;

  *fps = 0;

  // FIXME: Ignore zero adc for now.
  if (adc == 0) {
    return 0;
  }

  // Gather initial statistics
  if (state == FPS_STATE_INIT && timestamp_ms < 1000) {

    if (adc > high) {
      high = adc;
    }

    if (adc < low) {
      low = adc;
    }

    return 0;
  }

  // High limit = 75% of the range
  // Low limit = 25% of the range
  quarter_range = (high - low) >> 2;
  *high_limit = high - quarter_range;
  *low_limit = low + quarter_range;

  // Check if we are in a known state after initialisation
  if (state == FPS_STATE_INIT) {
    if (adc < *low_limit) {
      state = FPS_STATE_LOW;
    }
    if (adc > *high_limit) {
      state = FPS_STATE_HIGH;
    }

    // Even if we are now in a valid state, we have can have a change
    // next time earliest.
    return 0;
  }

  if (state == FPS_STATE_LOW) {

    // Frame has changed if we are in a low state and adc is high
    if (adc > *high_limit) {

      state = FPS_STATE_HIGH;
      is_new_frame = 1;

      if (new_low < *high_limit) {
        uint16_t change;
        uint8_t negative = 0;

        // Adjust low value 1/4 towards the latest low value
        if (new_low < low) {
          negative = 1;
          change = low - new_low;
        } else {
          change = new_low - low;
        }

        change >>= 2;

        // Make sure low doesn't overflow
        if (negative) {
          if (change < low) {
            low -= change;
          }
        } else {
          // FIXME: is the following temporary register 32bit?
          if (low + change < 0xffff) {
            low += change;
          }
        }

        // Reset low for the next low state
        new_low = 0xffff;
      }

    } else {
      // Get the most lowest value during this low state
      if (adc < new_low) {
        new_low = adc;
      }
    }
  }

  if (state == FPS_STATE_HIGH) {

    // Frame has changed if we are in a high state and adc is low
    if (adc < *low_limit) {

      state = FPS_STATE_LOW;
      is_new_frame = 1;

      if (new_high > *low_limit) {
        uint16_t change;
        uint8_t negative = 0;

        // Adjust high value 1/4 towards the latest high value
        if (new_high < high) {
          negative = 1;
          change = high - new_high;
        } else {
          change = new_high - high;
        }

        change >>= 2;

        // Make sure high doesn't overflow
        if (negative) {
          if (change < high) {
            high -= change;
          }
        } else {
          // FIXME: is the following temporary register 32bit?
          if (high + change < 0xffff) {
            high += change;
          }
        }

        // Reset high for the next high state
        new_high = 0;
      }
    } else {
      // Get the most highest value during this high state
      if (adc > new_high) {
        new_high = adc;
      }
    }
  }

  if (!is_new_frame) {
    return 0;
  }

  frames[frame_i] = timestamp_ms;

  // Count the amount of new frames during last 1000 ms
  for (i = 0; i < FPS_PREV_FRAMES; ++i) {
    if (timestamp_ms - frames[i] < 1000) {
      ++*fps;
    }
  }

  if (++frame_i == FPS_PREV_FRAMES) {
    frame_i = 0;
  }

  *low_ret = low;
  *high_ret = high;
  return 1;
}



/*
 * Construct a message and send it over the UART
 */
uint8_t create_message(uint8_t *buf,
                       uint8_t max_len,
                       uint32_t timestamp_ms,
                       uint8_t fps,
                       uint16_t missed_adc,
                       uint16_t frame_len,
                       uint16_t low,
                       uint16_t low_limit,
                       uint16_t high_limit,
                       uint16_t high)
{
  uint8_t len = 0;

  // FIXME: all this will fail is buf is too small
#define BUF_APPEND(a) len += sc_itoa(a, &buf[len], max_len - len); buf[len++] = ',';
  BUF_APPEND(timestamp_ms);
  BUF_APPEND(fps);
  BUF_APPEND(missed_adc);
  BUF_APPEND(frame_len);
  BUF_APPEND(low);
  BUF_APPEND(low_limit);
  BUF_APPEND(high_limit);
  BUF_APPEND(high);
#undef BUF_APPEND

  // Overwrite the last ,
  buf[len-1] = '\r';
  buf[len++] = '\n';

  return len;
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

