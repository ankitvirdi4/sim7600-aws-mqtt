/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#include "sim7600_port_teensy.h"

extern "C" {

void sim7600_port_teensy_init(sim7600_port_t *p,
                              const sim7600_port_teensy_config_t *cfg)
{
    p->uart       = cfg->uart;
    p->pwrkey_pin = cfg->pwrkey_pin;
    p->reset_pin  = cfg->reset_pin;
    p->status_pin = cfg->status_pin;
    p->at_ctx     = nullptr;

    pinMode(p->pwrkey_pin, OUTPUT);
    pinMode(p->reset_pin,  OUTPUT);
    pinMode(p->status_pin, INPUT);
    digitalWrite(p->pwrkey_pin, LOW);
    digitalWrite(p->reset_pin,  LOW);
}

void sim7600_port_teensy_attach_at(sim7600_port_t *p, sim7600_at_ctx_t *at)
{
    p->at_ctx = at;
}

void sim7600_port_teensy_poll(sim7600_port_t *p)
{
    if (p->at_ctx == nullptr || p->uart == nullptr) {
        return;
    }
    while (p->uart->available()) {
        uint8_t b = (uint8_t)p->uart->read();
        sim7600_at_feed(p->at_ctx, &b, 1);
    }
}

/* sim7600_port.h interface implementations. */

int sim7600_port_uart_init(sim7600_port_t *p, uint32_t baud)
{
    if (p->uart != nullptr) {
        p->uart->begin(baud);
    }
    return 0;
}

int sim7600_port_uart_write(sim7600_port_t *p, const uint8_t *buf, size_t len)
{
    if (p->uart == nullptr) {
        return -1;
    }
    size_t written = p->uart->write(buf, len);
    return (int)written;
}

int sim7600_port_uart_read(sim7600_port_t *p, uint8_t *buf, size_t maxlen)
{
    /* Not used; sim7600_port_teensy_poll feeds the AT ring buffer. */
    (void)p; (void)buf; (void)maxlen;
    return 0;
}

static int resolve_teensy_pin(sim7600_port_t *p, sim7600_pin_t pin)
{
    switch (pin) {
        case SIM7600_PIN_PWRKEY: return (int)p->pwrkey_pin;
        case SIM7600_PIN_RESET:  return (int)p->reset_pin;
        case SIM7600_PIN_STATUS: return (int)p->status_pin;
    }
    return -1;
}

void sim7600_port_gpio_set(sim7600_port_t *p, sim7600_pin_t pin, bool high)
{
    int tp = resolve_teensy_pin(p, pin);
    if (tp < 0) return;
    digitalWrite(tp, high ? HIGH : LOW);
}

bool sim7600_port_gpio_get(sim7600_port_t *p, sim7600_pin_t pin)
{
    int tp = resolve_teensy_pin(p, pin);
    if (tp < 0) return false;
    return digitalRead(tp) == HIGH;
}

uint32_t sim7600_port_millis(void)
{
    return millis();
}

void sim7600_port_delay_ms(uint32_t ms)
{
    delay(ms);
}

} /* extern "C" */
