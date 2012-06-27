#include "msp430.h"
#include "RF1A.h"
#include "hal_pmm.h"

/*******************
 * Function Definition
 */
int send_next_msg(void);
void RfTransmit(unsigned char *buffer, unsigned char length);
void RfReceiveOn(void);
void RfReceiveOff(void);

void InitLeds(void);
void InitRadio(void);
void InitUart(void);

void led_toggle(int led);
void led_off(int led);
void led_on(int led);
int sc_itoa(int value, unsigned char *str, int len);
void sleep_ms(int ms);
