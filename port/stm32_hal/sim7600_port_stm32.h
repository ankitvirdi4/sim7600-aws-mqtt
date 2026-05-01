/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#ifndef SIM7600_PORT_STM32_H
#define SIM7600_PORT_STM32_H

#include "sim7600_port.h"
#include "sim7600_at.h"
#include "stm32_hal_glue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sim7600_port_stm32.h
 * @brief Generic STM32 HAL implementation of the sim7600 port interface.
 *
 * Family agnostic: includes only "stm32_hal_glue.h", which the user's
 * project provides and which forwards to the correct family HAL umbrella
 * header (stm32f4xx_hal.h, stm32g4xx_hal.h, etc).
 *
 * UART RX uses DMA in idle line mode via HAL_UARTEx_ReceiveToIdle_DMA.
 * The application must wire HAL_UARTEx_RxEventCallback to call
 * sim7600_port_stm32_rx_event_cb. UART TX is blocking via HAL_UART_Transmit.
 */

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} sim7600_stm32_pin_t;

struct sim7600_port {
    UART_HandleTypeDef *huart;

    /* Circular DMA RX buffer. HAL fills this; we copy out new bytes. */
    uint8_t *dma_rx_buf;
    size_t   dma_rx_buf_size;
    size_t   dma_rx_pos;

    sim7600_stm32_pin_t pwrkey;
    sim7600_stm32_pin_t reset;
    sim7600_stm32_pin_t status_in;

    sim7600_at_ctx_t *at_ctx;
};

typedef struct {
    UART_HandleTypeDef *huart;
    uint8_t            *dma_rx_buf;
    size_t              dma_rx_buf_size;
    sim7600_stm32_pin_t pwrkey;
    sim7600_stm32_pin_t reset;
    sim7600_stm32_pin_t status_in;
} sim7600_port_stm32_config_t;

/**
 * @brief Initialise the port struct.
 *
 * Does not touch hardware. Call AFTER the user's project has initialised
 * the UART, GPIOs, and DMA via CubeMX generated MX_*_Init() calls.
 */
void sim7600_port_stm32_init(sim7600_port_t *p,
                             const sim7600_port_stm32_config_t *cfg);

/**
 * @brief Bind the AT engine context for byte feeding from the RX callback.
 *
 * Must be called AFTER sim7600_at_init() and BEFORE
 * sim7600_port_stm32_start_rx().
 */
void sim7600_port_stm32_attach_at(sim7600_port_t *p, sim7600_at_ctx_t *at);

/**
 * @brief Start DMA reception with idle line detection.
 *
 * Calls HAL_UARTEx_ReceiveToIdle_DMA. The user must wire
 * HAL_UARTEx_RxEventCallback to call sim7600_port_stm32_rx_event_cb().
 */
HAL_StatusTypeDef sim7600_port_stm32_start_rx(sim7600_port_t *p);

/**
 * @brief Hook to call from HAL_UARTEx_RxEventCallback.
 *
 * Pass the Size argument from that HAL callback. This function copies the
 * newly arrived bytes (since the previous callback) into the AT engine
 * ring buffer. The HAL keeps the DMA running in circular mode, no restart
 * is needed here.
 */
void sim7600_port_stm32_rx_event_cb(sim7600_port_t *p, uint16_t Size);

#ifdef __cplusplus
}
#endif

#endif /* SIM7600_PORT_STM32_H */
