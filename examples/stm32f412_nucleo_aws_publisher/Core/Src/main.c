/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 *
 * Nucleo-F412ZG example: power on the SIM7600, send AT every 2 seconds,
 * print the response on the ST-Link VCP (USART3). M2 echo test only;
 * no network, no TLS, no MQTT yet.
 */

#include "stm32f4xx_hal.h"

#include "sim7600_at.h"
#include "sim7600_port_stm32.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* HAL peripheral handles. */
static UART_HandleTypeDef huart2;       /* Modem UART. */
static UART_HandleTypeDef huart3;       /* ST-Link VCP for debug. */
static DMA_HandleTypeDef  hdma_usart2_rx;

/* Library state. */
static sim7600_port_t  port;
static sim7600_at_ctx_t at;
static uint8_t         dma_rx_buf[256];
static uint8_t         ring_buf[1024];

/* Forward declarations. */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void vcp_printf(const char *fmt, ...);
static void modem_power_on(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();

    sim7600_port_stm32_config_t cfg = {
        .huart           = &huart2,
        .dma_rx_buf      = dma_rx_buf,
        .dma_rx_buf_size = sizeof(dma_rx_buf),
        .pwrkey          = { GPIOA, GPIO_PIN_0 },
        .reset           = { GPIOA, GPIO_PIN_1 },
        .status_in       = { GPIOA, GPIO_PIN_4 },
    };
    sim7600_port_stm32_init(&port, &cfg);
    sim7600_at_init(&at, &port, ring_buf, sizeof(ring_buf));
    sim7600_port_stm32_attach_at(&port, &at);
    sim7600_port_stm32_start_rx(&port);

    modem_power_on();
    vcp_printf("sim7600 echo test starting\r\n");

    for (;;) {
        char resp[64];
        int rc;
        do {
            rc = sim7600_at_send_cmd(&at, "AT", NULL, 1000,
                                     resp, sizeof(resp));
            sim7600_at_task(&at);
        } while (rc == SIM7600_AT_PENDING);

        if (rc == SIM7600_AT_OK) {
            vcp_printf("AT -> OK\r\n");
        } else {
            vcp_printf("AT -> rc=%d\r\n", rc);
        }
        HAL_Delay(2000);
    }
}

/* HAL_UARTEx_RxEventCallback is __weak in the HAL; this overrides it. */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart2) {
        sim7600_port_stm32_rx_event_cb(&port, Size);
    }
}

/* IRQ handlers route to HAL. */
void DMA1_Stream5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

/* Power on sequence: hold PWRKEY low briefly, pulse high for >1s, low again,
 * then wait ~5s for the SIM7600 to boot. Pin polarity assumes the breakout
 * inverts PWRKEY (high level on this MCU pin pulls the modem PWRKEY low to
 * ground via a transistor). If your breakout is direct, swap the polarities.
 */
static void modem_power_on(void)
{
    sim7600_port_gpio_set(&port, SIM7600_PIN_RESET,  false);
    sim7600_port_gpio_set(&port, SIM7600_PIN_PWRKEY, false);
    HAL_Delay(100);
    sim7600_port_gpio_set(&port, SIM7600_PIN_PWRKEY, true);
    HAL_Delay(1100);
    sim7600_port_gpio_set(&port, SIM7600_PIN_PWRKEY, false);
    HAL_Delay(5000);
}

/* HSI 16 MHz -> PLLM 8 -> 2 MHz -> PLLN 100 -> 200 MHz -> PLLP 2 -> 100 MHz. */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM            = 8;
    osc.PLL.PLLN            = 100;
    osc.PLL.PLLP            = RCC_PLLP_DIV2;
    osc.PLL.PLLQ            = 4;
    HAL_RCC_OscConfig(&osc);

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_3);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef io = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);

    /* PWRKEY (PA0) and RESET (PA1) outputs. */
    io.Pin   = GPIO_PIN_0 | GPIO_PIN_1;
    io.Mode  = GPIO_MODE_OUTPUT_PP;
    io.Pull  = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &io);

    /* STATUS (PA4) input with pull down. */
    io.Pin  = GPIO_PIN_4;
    io.Mode = GPIO_MODE_INPUT;
    io.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &io);
}

static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

static void MX_USART3_UART_Init(void)
{
    huart3.Instance          = USART3;
    huart3.Init.BaudRate     = 115200;
    huart3.Init.WordLength   = UART_WORDLENGTH_8B;
    huart3.Init.StopBits     = UART_STOPBITS_1;
    huart3.Init.Parity       = UART_PARITY_NONE;
    huart3.Init.Mode         = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart3);
}

/* HAL_UART_MspInit configures GPIO alternate function and DMA per UART. */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef io = {0};

    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        io.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        io.Mode      = GPIO_MODE_AF_PP;
        io.Pull      = GPIO_NOPULL;
        io.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        io.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &io);

        hdma_usart2_rx.Instance                 = DMA1_Stream5;
        hdma_usart2_rx.Init.Channel             = DMA_CHANNEL_4;
        hdma_usart2_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hdma_usart2_rx.Init.Mode                = DMA_CIRCULAR;
        hdma_usart2_rx.Init.Priority            = DMA_PRIORITY_HIGH;
        hdma_usart2_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
        HAL_DMA_Init(&hdma_usart2_rx);
        __HAL_LINKDMA(huart, hdmarx, hdma_usart2_rx);

        HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    } else if (huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        io.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
        io.Mode      = GPIO_MODE_AF_PP;
        io.Pull      = GPIO_NOPULL;
        io.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        io.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOD, &io);
    }
}

static void vcp_printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) {
        HAL_UART_Transmit(&huart3, (uint8_t *)buf, (uint16_t)n, 1000);
    }
}

void Error_Handler(void)
{
    for (;;) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file; (void)line;
    Error_Handler();
}
#endif
