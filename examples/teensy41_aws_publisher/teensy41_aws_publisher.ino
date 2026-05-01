/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 *
 * Teensy 4.1 example: power on the SIM7600, send AT every 2 seconds,
 * print the response on USB Serial. M3 echo test only; no network, no
 * TLS, no MQTT yet.
 */

#include "sim7600_at.h"
#include "sim7600_port_teensy.h"

static sim7600_port_t   port;
static sim7600_at_ctx_t at;
static uint8_t          ring_buf[1024];

void setup()
{
    Serial.begin(115200);   /* USB Serial for debug. */
    while (!Serial && millis() < 3000) { /* wait briefly for the host port. */ }

    Serial1.begin(115200);  /* Modem UART. */

    sim7600_port_teensy_config_t cfg;
    cfg.uart       = &Serial1;
    cfg.pwrkey_pin = 2;
    cfg.reset_pin  = 3;
    cfg.status_pin = 4;
    sim7600_port_teensy_init(&port, &cfg);

    sim7600_at_init(&at, &port, ring_buf, sizeof(ring_buf));
    sim7600_port_teensy_attach_at(&port, &at);

    /* Power on sequence: PWRKEY low, pulse high > 1s, low, wait for boot. */
    sim7600_port_gpio_set(&port, SIM7600_PIN_RESET,  false);
    sim7600_port_gpio_set(&port, SIM7600_PIN_PWRKEY, false);
    delay(100);
    sim7600_port_gpio_set(&port, SIM7600_PIN_PWRKEY, true);
    delay(1100);
    sim7600_port_gpio_set(&port, SIM7600_PIN_PWRKEY, false);
    delay(5000);

    Serial.println("sim7600 echo test starting");
}

void loop()
{
    sim7600_port_teensy_poll(&port);

    char resp[64];
    int rc;
    do {
        rc = sim7600_at_send_cmd(&at, "AT", nullptr, 1000,
                                 resp, sizeof(resp));
        sim7600_at_task(&at);
        sim7600_port_teensy_poll(&port);
    } while (rc == SIM7600_AT_PENDING);

    if (rc == SIM7600_AT_OK) {
        Serial.println("AT -> OK");
    } else {
        Serial.printf("AT -> rc=%d\n", rc);
    }
    delay(2000);
}
