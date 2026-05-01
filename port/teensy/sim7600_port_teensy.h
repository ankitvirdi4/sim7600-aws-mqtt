/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#ifndef SIM7600_PORT_TEENSY_H
#define SIM7600_PORT_TEENSY_H

/*
 * This header is C++ only. It references HardwareSerial from Arduino.h.
 * Teensy and Arduino projects are inherently C++ so this is not a
 * limitation in practice.
 */
#ifndef __cplusplus
#error "sim7600_port_teensy.h is C++ only; include from a C++ source"
#endif

#include <Arduino.h>
#include "sim7600_port.h"
#include "sim7600_at.h"

typedef struct {
    HardwareSerial *uart;
    uint8_t pwrkey_pin;
    uint8_t reset_pin;
    uint8_t status_pin;
} sim7600_port_teensy_config_t;

struct sim7600_port {
    HardwareSerial *uart;
    uint8_t pwrkey_pin;
    uint8_t reset_pin;
    uint8_t status_pin;
    sim7600_at_ctx_t *at_ctx;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise the port struct and configure the GPIO pin modes.
 *
 * Does NOT call uart->begin(); the user calls Serial1.begin(115200) in
 * setup() before this. Drives PWRKEY and RESET low so the modem stays off
 * until the user runs the power on sequence.
 */
void sim7600_port_teensy_init(sim7600_port_t *p,
                              const sim7600_port_teensy_config_t *cfg);

/**
 * @brief Bind the AT engine context for byte feeding from poll().
 *
 * Must be called AFTER sim7600_at_init() and BEFORE the first
 * sim7600_port_teensy_poll().
 */
void sim7600_port_teensy_attach_at(sim7600_port_t *p, sim7600_at_ctx_t *at);

/**
 * @brief Drain available bytes from the modem UART into the AT ring buffer.
 *
 * Call this from loop() at least as often as the AT engine runs. The
 * Teensy 4.1 HardwareSerial software RX buffer is around 64 bytes, so
 * any loop() iteration faster than 60 ms at 115200 baud avoids overflow.
 * For higher throughput phases (cert upload at M5), consider lowering the
 * delay() between iterations or moving to a DMA based RX in a future
 * v0.2.0 port revision.
 */
void sim7600_port_teensy_poll(sim7600_port_t *p);

#ifdef __cplusplus
}
#endif

#endif /* SIM7600_PORT_TEENSY_H */
