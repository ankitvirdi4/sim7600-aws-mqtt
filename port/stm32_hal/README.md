# STM32 generic HAL port

A platform port of sim7600-aws-mqtt that targets the generic STM32 HAL API. Compiles against any STM32 family (F0, F1, F3, F4, F7, G0, G4, H7, L0, L1, L4, L5, U5, WB, WL) without family specific `#ifdef`. The reference example is on a Nucleo-F412ZG; other STM32 boards should work and PRs adding tested rows to the table below are welcome.

## How it works

* UART RX uses HAL_UARTEx_ReceiveToIdle_DMA in circular mode. On every idle line event the HAL fires HAL_UARTEx_RxEventCallback. The application wires that callback to sim7600_port_stm32_rx_event_cb, which copies new bytes into the AT engine ring buffer.
* UART TX uses HAL_UART_Transmit (blocking) for v0.1.0. A non blocking DMA TX path is a candidate for v0.2.0.
* GPIO uses HAL_GPIO_WritePin and HAL_GPIO_ReadPin.
* sim7600_port_millis returns HAL_GetTick(); sim7600_port_delay_ms calls HAL_Delay() and is used only for power sequencing.

## Family glue header

The port code includes `stm32_hal_glue.h`. The user's project provides this header and points it at the correct family HAL umbrella:

```c
/* Your project's stm32_hal_glue.h, F4 example. */
#ifndef STM32_HAL_GLUE_H
#define STM32_HAL_GLUE_H
#include "stm32f4xx_hal.h"
#endif
```

For G4 swap to `stm32g4xx_hal.h`, for H7 to `stm32h7xx_hal.h`, and so on. This is the only family decision in the project; the port itself never names a family.

## CubeMX configuration

This is the minimum CubeMX setup the example example expects. Adjust pins for your board.

| Peripheral | Mode | Settings |
|---|---|---|
| USART2 | Asynchronous | 115200, 8N1, no flow control |
| USART2 RX DMA | DMA1 Stream 5 (F4 family) or equivalent | Circular, byte to byte, peripheral to memory |
| USART2 IRQ | NVIC enabled | Required for idle line interrupt |
| USART2 RX | (family pin) | AF for USART2 (AF7 on F4) |
| USART2 TX | (family pin) | AF for USART2 |
| Modem PWRKEY | GPIO output, low default | Push pull, low speed |
| Modem RESET | GPIO output, low default | Push pull, low speed |
| Modem STATUS | GPIO input, pull down | |
| SYSCLK | family appropriate | Anything 24 MHz or above is fine for 115200 |

Bigger families (H7) and smaller ones (F0) shift the DMA stream and channel numbers; CubeMX picks the right ones automatically once USART2 RX DMA is enabled.

## Wire up the RX callback

In the user code section of your main.c (or any .c that the linker sees), define:

```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    extern sim7600_port_t modem_port; /* or wherever your sim7600_port_t lives */
    if (huart == &huart2) {
        sim7600_port_stm32_rx_event_cb(&modem_port, Size);
    }
}
```

CubeMX leaves HAL_UARTEx_RxEventCallback as a `__weak` stub. Defining it in your code overrides that.

## Initialisation order

```c
HAL_Init();
SystemClock_Config();
MX_GPIO_Init();
MX_DMA_Init();
MX_USARTx_UART_Init();

sim7600_port_stm32_config_t cfg = { /* huart, dma buf, pins */ };
sim7600_port_stm32_init(&port, &cfg);
sim7600_at_init(&at, &port, ring_buf, sizeof(ring_buf));
sim7600_port_stm32_attach_at(&port, &at);
sim7600_port_stm32_start_rx(&port);
```

After this, sim7600_at_send_cmd and sim7600_at_task work as documented in core/sim7600_at.h.

## Tested boards

| Board | MCU | HAL family | Status |
|---|---|---|---|
| NUCLEO-F412ZG | STM32F412ZG | F4 | reference example, tested |

Other STM32 boards should work but are untested. Adding a row here with a wiring diagram and a PR is the easiest contribution.
