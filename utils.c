/*
 * Utility functions
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

#include "utils.h"
#include "stdint.h"



/*
 * Convert int to string. Returns length on the converted string
 * (excluding the trailing \0) or 0 in error.
 */
unsigned char sc_itoa(int32_t value, unsigned char *str, unsigned char len)
{
  unsigned char index = len;
  unsigned char extra, i;
  unsigned char str_len;
  unsigned char negative = 0;
  unsigned char start = 0;

  // Check for negative values
  if (value < 0) {
    negative = 1;
    value *= -1;
  }

  // Go through all numbers and store the last digit to the string
  // starting for the last character
  while (--index >= 0) {
    str[index] = (value % 10) + '0';
    if (value < 10) {
      break;
    }
    value /= 10;
  }

  // Check for overflow (needs also space for '\0' and possible '-').
  if (index < 1 || (negative && index < 2)) {
    str[0] = '\0';
    return 0;
  }

  // Add - for negative numbers
  if (negative) {
    str[0] = '-';
    start = 1;
  }

  // Move numbers to the start of the string. Add \0 after the last
  // number or as the last character in case of overflow
  extra = index;
  for (i = 0; i < len - extra; i++) {
    str[i + start] = str[i + extra];
  }
  str_len = len - extra + start;

  str[str_len] = '\0';

  return str_len;
}



/*
 * Busy loop sleep ms milliseconds.
 */
void busysleep_ms(int ms)
{
  int a;
  for (a = 0; a < ms; a++) {
    __delay_cycles(1000);
  }
}




/*
 * Busy loop sleep ms microseconds.
 */
void busysleep_us(int us)
{
  int a;
  for (a = 0; a < us; a++) {
    __delay_cycles(1);
  }
}




/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
