#include <stdint.h>
#include <stdio.h>

#include "stm32f446xx.h"
#include "stm32f446xx_dma.h"
#include "stm32f446xx_gpio.h"
#include "stm32f446xx_usart.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
#warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

void setup_uart();

int _write(int le, char* ptr, int len) {
  int DataIdx;
  for (DataIdx = 0; DataIdx < len; DataIdx++) {
    ITM_SendChar(*ptr++);
  }
  return len;
}

#define SIZEOF(arr) ((unsigned int)sizeof(arr) / sizeof(arr[0]))

#define VERY_FAST 16
#define FAST 100000
#define MEDIUM 300001
#define SLOW 1000000

#define UART_GPIO_PORT GPIOC
#define UART_GPIO_TX_PIN 10
#define UART_GPIO_RX_PIN 11
#define UART_GPIO_ALTFN_NUM 8
#define UART_PORT UART4

#define WAIT(CNT)                                          \
  do {                                                     \
    for (int sleep_cnt = 0; sleep_cnt < CNT; sleep_cnt++); \
  } while (0)

int main(void) {
  char test_str[] = "1a2a3a";
  setup_uart();

  int breakpoint_here = 0;

  for (;;) {
    WAIT(MEDIUM);
    // NOTE: Next step: Take something from the rx buffer, convert it to int, add one, tx it
    uint8_t rx_ret_byte = '6';
    rx_ret_byte = usart_rx_byte_blocking(UART_PORT) - '0';

    // Reconvert back to char after adding 1, send'er
    rx_ret_byte = rx_ret_byte + 1 + '0';
    usart_tx_byte_blocking(UART_PORT, rx_ret_byte);
  }
}

void setup_uart() {
  GPIO_peri_clock_control(UART_GPIO_PORT, GPIO_CLOCK_ENABLE);
  GPIOConfig_t default_gpio_cfg = {.mode = GPIO_MODE_ALTFN,
                                   .speed = GPIO_SPEED_HIGH,
                                   .float_resistor = GPIO_PUPDR_NONE,
                                   .output_type = GPIO_OP_TYPE_PUSHPULL,
                                   .alt_func_num = UART_GPIO_ALTFN_NUM};

  GPIOHandle_t uart_tx = {.p_GPIO_addr = UART_GPIO_PORT, .cfg = default_gpio_cfg};
  uart_tx.cfg.pin_number = UART_GPIO_TX_PIN;
  GPIO_init(&uart_tx);

  GPIOHandle_t uart_rx = {.p_GPIO_addr = UART_GPIO_PORT, .cfg = default_gpio_cfg};
  uart_rx.cfg.pin_number = UART_GPIO_RX_PIN;
  GPIO_init(&uart_rx);

  usart_peri_clock_control(UART_PORT, USART_ENABLE);
  USARTConfig_t uart_cfg = {.baud_rate = USART_BAUD_RATE_9600,
                            .peri_clock_freq_hz = 16E6,
                            .en_on_start = USART_ENABLE,
                            .mode = USART_MODE_BIDIRECTIONAL,
                            .hw_flow_control = USART_HW_FLOW_NONE,
                            .parity_type = USART_PARITY_NONE,
                            .stop_bit_count = USART_STOP_BITS_ONE,
                            .word_length = USART_WORD_LENGTH_8_BIT_DATA,
                            .synchronous = USART_ASYNCHRONOUS};

  USARTHandle_t usart_handle = {.addr = UART_PORT, .cfg = uart_cfg};
  usart_init(&usart_handle);
}
