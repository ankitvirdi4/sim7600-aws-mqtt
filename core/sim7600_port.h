/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#ifndef SIM7600_PORT_H
#define SIM7600_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sim7600_port.h
 * @brief Platform interface contract.
 *
 * Each platform (STM32 HAL, Teensy/Arduino, host mock) defines the
 * concrete sim7600_port_t struct and implements the functions below.
 * Core code only sees the opaque pointer and the function signatures.
 */

/** Opaque platform port type; defined per platform. */
typedef struct sim7600_port sim7600_port_t;

/** Logical pins exposed by every platform. */
typedef enum {
    SIM7600_PIN_PWRKEY = 0,
    SIM7600_PIN_RESET  = 1,
    SIM7600_PIN_STATUS = 2,
} sim7600_pin_t;

/**
 * @brief Initialise the modem UART at the requested baud.
 * @return 0 on success, negative on failure.
 */
int  sim7600_port_uart_init(sim7600_port_t *p, uint32_t baud);

/**
 * @brief Transmit raw bytes on the modem UART.
 * @return number of bytes accepted, or negative on error.
 */
int  sim7600_port_uart_write(sim7600_port_t *p, const uint8_t *buf, size_t len);

/**
 * @brief Optional polling read for ports without DMA. Not used by the
 *        AT engine when the platform feeds bytes via sim7600_at_feed().
 * @return number of bytes read, 0 if none available, negative on error.
 */
int  sim7600_port_uart_read(sim7600_port_t *p, uint8_t *buf, size_t maxlen);

/** Drive a logical pin high or low. */
void sim7600_port_gpio_set(sim7600_port_t *p, sim7600_pin_t pin, bool high);

/** Read the level of a logical pin. */
bool sim7600_port_gpio_get(sim7600_port_t *p, sim7600_pin_t pin);

/** Free running millisecond counter. */
uint32_t sim7600_port_millis(void);

/** Block for the given number of milliseconds. Use only for power sequencing. */
void     sim7600_port_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* SIM7600_PORT_H */
