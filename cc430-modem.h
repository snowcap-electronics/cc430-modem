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
unsigned char sc_itoa(int value, unsigned char *str, unsigned char len);
void sleep_ms(int ms);
void handle_rf_rx_packet(void);
int handle_uart_rx_byte(void);
void timer_set(int ms);
void timer_clear(void);
