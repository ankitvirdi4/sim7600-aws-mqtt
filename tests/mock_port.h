/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#ifndef MOCK_PORT_H
#define MOCK_PORT_H

#include <stddef.h>
#include <stdint.h>

#include "sim7600_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Get a handle to the singleton mock port for use with sim7600_at_init. */
sim7600_port_t *mock_port_handle(void);

/* Reset the mock state between tests. */
void mock_port_reset(void);

/* Manipulate the mock millisecond clock. */
void mock_port_set_millis(uint32_t ms);
void mock_port_advance_ms(uint32_t ms);

/* Inspect bytes that the AT engine "sent" via sim7600_port_uart_write. */
size_t          mock_port_get_tx_len(void);
const uint8_t  *mock_port_get_tx_buf(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_PORT_H */
