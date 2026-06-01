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

uint8_t tx_buff[10] = "";
uint16_t tx_len = 10;
uint8_t rx_buff[10] = "";
uint16_t rx_len = 10;

/* NOTE: I should look into a command like structure for rx ...
          Maybe have USARTRXCommand_t ...
*/

/* NOTE: Probably should also move rx_interrupt_enable to the setup rx interrupt stuff */
/* NOTE: Maybe also add an enable flag to make the config more explicit */

void rx_callback(void) {
  for (int i = 0; i < rx_len; i++) {
    tx_buff[i] = 10 - (rx_buff[i] - '0');
    tx_buff[i] += '0';
  }

  usart_start_tx_interrupt(UART_PORT);
}

int main(void) {
  setup_uart();

  USARTInterruptConfig_t usart_int_setup_cfg = {
      .error_interrupts_en = USART_DISABLE,
      .idle_en = USART_DISABLE,
      .tx_complete_en = USART_DISABLE,
      .tx = {.buff = tx_buff, .len = tx_len, .circular = USART_INTERRUPT_NON_CIRCULAR, .callback = NULL},
      .rx = {.buff = rx_buff, .len = rx_len, .callback = rx_callback}};
  usart_setup_interrupt(UART_PORT, &usart_int_setup_cfg);

  NVIC_EnableIRQ(UART4_IRQn);

  for (;;) {
    WAIT(SLOW);
  }
}

void UART4_IRQHandler(void) {
  USARTIRQType_t irq_type = usart_irq_handling(UART4);
  if (irq_type == USART_IRQ_TYPE_TXE) {
    usart_irq_tx_word_handling(UART_PORT);
  } else if (irq_type == USART_IRQ_TYPE_RXNE) {
    usart_irq_rx_word_handling(UART_PORT);
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
                            .rx_interrupt_en = USART_ENABLE,
                            .synchronous = USART_ASYNCHRONOUS};

  USARTHandle_t usart_handle = {.addr = UART_PORT, .cfg = uart_cfg};
  usart_init(&usart_handle);
}
