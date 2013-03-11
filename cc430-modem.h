#include "msp430.h"
#include "RF1A.h"
#include "hal_pmm.h"

#include <stdint.h>

enum adc_state_t {
  ADC_IDLE = 0,
  ADC_START_PENDING,
  ADC_MEASURING,
  ADC_DATA
};

enum i2c_state_t {
  I2C_IDLE = 0,
  I2C_ACTIVE,
  I2C_READ_DATA
};

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

void adc_start(unsigned char chan);
void adc_shutdown(void);
void i2c_pinmux(void);
void i2c_reset(void);
void i2c_send(unsigned char *buf, unsigned char bytes);
uint16_t i2c_read(void);
