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

#define USART_DMA_TX_PORT DMA1
#define USART_DMA_TX_STREAM DMA1_Stream4
#define USART_DMA_TX_CHANNEL 4
#define USART_DMA_TX_STREAM_IRQN DMA1_Stream4_IRQn
#define USART_DMA_TX_STREAM_IRQ_HANDLER DMA1_Stream4_IRQHandler

#define USART_DMA_RX_PORT DMA1
#define USART_DMA_RX_STREAM DMA1_Stream2
#define USART_DMA_RX_CHANNEL 4
#define USART_DMA_RX_STREAM_IRQN DMA1_Stream2_IRQn
#define USART_DMA_RX_STREAM_IRQ_HANDLER DMA1_Stream2_IRQHandler

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

void tx_callback(void) { usart_reset_rx_interrupt(UART_PORT); }

void rx_callback(void) {
  rx_len = usart_get_rx_interrupt_length(UART_PORT);

  for (int i = 0; i < rx_len; i++) {
    uint8_t tx_byte = rx_buff[i] - '0';
    tx_byte = 9 - tx_byte;
    tx_byte += '0';

    tx_buff[i] = tx_byte;
  }

  usart_set_tx_interrupt_length(UART_PORT, rx_len);
  usart_reset_tx_interrupt(UART_PORT);
  usart_start_tx_interrupt(UART_PORT);
}

int main(void) {
  setup_uart();
  for (;;) {
    WAIT(SLOW);
  }
}

void USART_DMA_RX_STREAM_IRQ_HANDLER(void) {
  if (dma_irq_handling(USART_DMA_RX_STREAM, DMA_INTERRUPT_TYPE_FULL_TRANSFER_COMPLETE)) {
    for (int i = 0; i < rx_len; i++) {
      uint8_t byte = rx_buff[i] - 1;
      tx_buff[i] = byte;
    }
    dma_start_transfer(USART_DMA_TX_STREAM, tx_len);
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

  // NOTE: START OF DMA
  dma_peri_clock_control(USART_DMA_TX_PORT, DMA_ENABLE);
  DMAHandle_t dma_tx_handle = {
      .stream_addr = USART_DMA_TX_STREAM,
      .cfg = {.in = {.addr = (uint32_t*)tx_buff, .type = DMA_IO_TYPE_MEMORY, .inc = DMA_IO_ARR_INCREMENT},
              .out = {.addr = &UART_PORT->DR, .type = DMA_IO_TYPE_PERIPHERAL, .inc = DMA_IO_ARR_STATIC},
              .mem_data_size = DMA_DATA_SIZE_8_BIT,
              .peri_data_size = DMA_DATA_SIZE_8_BIT,
              .dma_elements = tx_len,
              .channel = USART_DMA_TX_CHANNEL,
              .priority = DMA_PRIORITY_HIGH,
              .circ_buffer = DMA_BUFFER_FINITE,
              .flow_control = DMA_PERIPH_NO_FLOW_CONTROL,
              .interrupt_en =
                  {
                      .direct_mode_error = DMA_DISABLE,
                      .transfer_error = DMA_DISABLE,
                      .full_transfer = DMA_DISABLE,
                      .half_transfer = DMA_DISABLE,
                  },
              .start_enabled = DMA_DISABLE},
  };
  dma_stream_init(&dma_tx_handle);

  dma_peri_clock_control(USART_DMA_RX_PORT, DMA_ENABLE);
  DMAHandle_t dma_rx_handle = {
      .cfg = {.in = {.addr = &UART_PORT->DR, .type = DMA_IO_TYPE_PERIPHERAL, .inc = DMA_IO_ARR_STATIC},
              .out = {.addr = (uint32_t*)rx_buff, .type = DMA_IO_TYPE_MEMORY, .inc = DMA_IO_ARR_INCREMENT},
              .mem_data_size = DMA_DATA_SIZE_8_BIT,
              .peri_data_size = DMA_DATA_SIZE_8_BIT,
              .dma_elements = rx_len,
              .channel = USART_DMA_RX_CHANNEL,
              .priority = DMA_PRIORITY_MAX,
              .circ_buffer = DMA_BUFFER_CIRCULAR,
              .flow_control = DMA_PERIPH_NO_FLOW_CONTROL,
              .interrupt_en =
                  {
                      .direct_mode_error = DMA_DISABLE,
                      .transfer_error = DMA_DISABLE,
                      .full_transfer = DMA_ENABLE,
                      .half_transfer = DMA_DISABLE,
                  },
              .start_enabled = DMA_ENABLE},
      .stream_addr = USART_DMA_RX_STREAM};
  dma_stream_init(&dma_rx_handle);
  NVIC_EnableIRQ(USART_DMA_RX_STREAM_IRQN);
  // NOTE: END OF DMA

  usart_peri_clock_control(UART_PORT, USART_ENABLE);
  USARTConfig_t uart_cfg = {.baud_rate = USART_BAUD_RATE_9600,
                            .peri_clock_freq_hz = 16E6,
                            .en_on_start = USART_ENABLE,
                            .tx_dma_en = USART_ENABLE,
                            .rx_dma_en = USART_ENABLE,
                            .mode = USART_MODE_BIDIRECTIONAL,
                            .hw_flow_control = USART_HW_FLOW_NONE,
                            .parity_type = USART_PARITY_NONE,
                            .stop_bit_count = USART_STOP_BITS_ONE,
                            .word_length = USART_WORD_LENGTH_8_BIT_DATA,
                            .synchronous = USART_ASYNCHRONOUS};

  USARTHandle_t usart_handle = {.addr = UART_PORT, .cfg = uart_cfg};
  usart_init(&usart_handle);
}
