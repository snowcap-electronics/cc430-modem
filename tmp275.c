/*
 * TMP275 temperature sensor
 *
 * Copyright 2013 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 * Copyright 2012 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
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

#include "tmp275.h"
#include "i2c.h"

void tmp275_start_oneshot(void)
{
    unsigned char tx_data[2];

    /* Pointer to Configuration register */
    tx_data[0] = 0x1;

    /* Configuration:
       Shutdown mode: 1
       Thermostat mode: 0
       Polarity: 0
       Fault queue: 00
       Resolution: 11 (12 bits)
       One-shot: 1
    */
    tx_data[1] = 0b11100000;
    i2c_send(tx_data, 2);

    // FIXME: move out side this function?
    __delay_cycles(50);                    // Delay required between transaction

    /* Set pointer to temperature register here so we can read without writing */
    tx_data[0] = 0x0;
    i2c_send(tx_data, 1);
}

void tmp275_shutdown(void)
{
    unsigned char tx_data[2];

    /* Pointer to Configuration register */
    tx_data[0] = 0x1;

    /* Configuration:
       Shutdown mode: 0
       Thermostat mode: 0
       Polarity: 0
       Fault queue: 00
       Resolution: 11 (12 bits)
       One-shot: 1
    */
    tx_data[1] = 0b01100001;
    i2c_send(tx_data, 2);

    // FIXME: move out side this function?
    __delay_cycles(50);                    // Delay required between transaction

    /* Set pointer to temperature register here so we can read without writing */
    tx_data[0] = 0x0;
    i2c_send(tx_data, 1);
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

