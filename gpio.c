/*
 * GPIO functionality
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

#include "gpio.h"

static uint16_t BITS[] = {BIT0, BIT1, BIT2, BIT3, BIT4, BIT5, BIT6, BIT7};

void gpio_on(uint8_t port, uint8_t pin)
{
  switch (port) {
  case 1:
	P1OUT |= BITS[pin];
	break;
  case 2:
	P2OUT |= BITS[pin];
	break;
  case 3:
	P3OUT |= BITS[pin];
	break;
  default:
	// Do nothing
	break;
  }
}



void gpio_off(uint8_t port, uint8_t pin)
{
  switch (port) {
  case 1:
	P1OUT &= ~BITS[pin];
	break;
  case 2:
	P2OUT &= ~BITS[pin];
	break;
  case 3:
	P3OUT &= ~BITS[pin];
	break;
  default:
	// Do nothing
	break;
  }
}



void gpio_dir(uint8_t port, uint8_t pin, GPIO_DIR dir)
{
  if (dir == GPIO_DIR_OUT) {
	switch (port) {
	case 1:
	  P1DIR |= BITS[pin];
	  break;
	case 2:
	  P2DIR |= BITS[pin];
	  break;
	case 3:
	  P3DIR |= BITS[pin];
	  break;
	default:
	  // Do nothing
	  break;
	}
  } else {
	switch (port) {
	case 1:
	  P1DIR &= ~BITS[pin];
	  break;
	case 2:
	  P2DIR &= ~BITS[pin];
	  break;
	case 3:
	  P3DIR &= ~BITS[pin];
	  break;
	default:
	  // Do nothing
	  break;
	}
  }
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

