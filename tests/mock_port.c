/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#include "mock_port.h"
#include "sim7600_port.h"

#include <string.h>

#define MOCK_TX_CAPACITY 1024

/* Concrete definition of the opaque sim7600_port_t for the host mock. */
struct sim7600_port {
    int dummy;
};

static struct sim7600_port g_mock_port;
static uint32_t            g_millis;
static uint8_t             g_tx_buf[MOCK_TX_CAPACITY];
static size_t              g_tx_len;

sim7600_port_t *mock_port_handle(void)
{
    return &g_mock_port;
}

void mock_port_reset(void)
{
    g_millis = 0;
    g_tx_len = 0;
}

void mock_port_set_millis(uint32_t ms)
{
    g_millis = ms;
}

void mock_port_advance_ms(uint32_t ms)
{
    g_millis += ms;
}

size_t mock_port_get_tx_len(void)
{
    return g_tx_len;
}

const uint8_t *mock_port_get_tx_buf(void)
{
    return g_tx_buf;
}

/* sim7600_port.h interface (host mock implementation). */

int sim7600_port_uart_init(sim7600_port_t *p, uint32_t baud)
{
    (void)p; (void)baud;
    return 0;
}

int sim7600_port_uart_write(sim7600_port_t *p, const uint8_t *buf, size_t len)
{
    (void)p;
    size_t free = MOCK_TX_CAPACITY - g_tx_len;
    size_t n    = (len < free) ? len : free;
    memcpy(g_tx_buf + g_tx_len, buf, n);
    g_tx_len += n;
    return (int)n;
}

int sim7600_port_uart_read(sim7600_port_t *p, uint8_t *buf, size_t maxlen)
{
    (void)p; (void)buf; (void)maxlen;
    return 0;
}

void sim7600_port_gpio_set(sim7600_port_t *p, sim7600_pin_t pin, bool high)
{
    (void)p; (void)pin; (void)high;
}

bool sim7600_port_gpio_get(sim7600_port_t *p, sim7600_pin_t pin)
{
    (void)p; (void)pin;
    return false;
}

uint32_t sim7600_port_millis(void)
{
    return g_millis;
}

void sim7600_port_delay_ms(uint32_t ms)
{
    g_millis += ms;
}
