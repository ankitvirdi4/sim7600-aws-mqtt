/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#include "sim7600_at.h"
#include "sim7600_port.h"
#include "mock_port.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RXBUF_SIZE 256

static int  g_urc_count;
static char g_last_urc[128];

static void urc_handler(const char *line, void *user)
{
    (void)user;
    g_urc_count++;
    strncpy(g_last_urc, line, sizeof(g_last_urc) - 1);
    g_last_urc[sizeof(g_last_urc) - 1] = '\0';
}

static int run_until_done(sim7600_at_ctx_t *ctx, const char *cmd,
                          const char *expected, uint32_t timeout,
                          char *resp, size_t resp_max,
                          uint32_t step_ms, uint32_t max_steps)
{
    int rc = sim7600_at_send_cmd(ctx, cmd, expected, timeout, resp, resp_max);
    for (uint32_t i = 0; i < max_steps && rc == SIM7600_AT_PENDING; i++) {
        sim7600_at_task(ctx);
        mock_port_advance_ms(step_ms);
        rc = sim7600_at_send_cmd(ctx, cmd, expected, timeout, resp, resp_max);
    }
    return rc;
}

static void test_ok(void)
{
    sim7600_at_ctx_t ctx;
    uint8_t rxbuf[RXBUF_SIZE];
    char resp[64];

    mock_port_reset();
    assert(sim7600_at_init(&ctx, mock_port_handle(), rxbuf, RXBUF_SIZE) == 0);

    int rc = sim7600_at_send_cmd(&ctx, "AT", NULL, 1000, resp, sizeof(resp));
    assert(rc == SIM7600_AT_PENDING);
    /* Mock UART captured the bytes that were "sent". */
    assert(mock_port_get_tx_len() == 4);
    assert(memcmp(mock_port_get_tx_buf(), "AT\r\n", 4) == 0);

    sim7600_at_feed(&ctx, (const uint8_t *)"OK\r\n", 4);
    sim7600_at_task(&ctx);

    rc = sim7600_at_send_cmd(&ctx, "AT", NULL, 1000, resp, sizeof(resp));
    assert(rc == SIM7600_AT_OK);
    printf("  ok: PASS\n");
}

static void test_error(void)
{
    sim7600_at_ctx_t ctx;
    uint8_t rxbuf[RXBUF_SIZE];

    mock_port_reset();
    sim7600_at_init(&ctx, mock_port_handle(), rxbuf, RXBUF_SIZE);

    sim7600_at_send_cmd(&ctx, "AT+BOGUS", NULL, 1000, NULL, 0);
    sim7600_at_feed(&ctx, (const uint8_t *)"ERROR\r\n", 7);
    sim7600_at_task(&ctx);
    int rc = sim7600_at_send_cmd(&ctx, "AT+BOGUS", NULL, 1000, NULL, 0);
    assert(rc == SIM7600_AT_ERROR);

    /* +CME ERROR variant. */
    mock_port_reset();
    sim7600_at_init(&ctx, mock_port_handle(), rxbuf, RXBUF_SIZE);
    sim7600_at_send_cmd(&ctx, "AT+CMEE?", NULL, 1000, NULL, 0);
    sim7600_at_feed(&ctx, (const uint8_t *)"+CME ERROR: 100\r\n", 17);
    sim7600_at_task(&ctx);
    rc = sim7600_at_send_cmd(&ctx, "AT+CMEE?", NULL, 1000, NULL, 0);
    assert(rc == SIM7600_AT_ERROR);
    printf("  error: PASS\n");
}

static void test_timeout(void)
{
    sim7600_at_ctx_t ctx;
    uint8_t rxbuf[RXBUF_SIZE];

    mock_port_reset();
    sim7600_at_init(&ctx, mock_port_handle(), rxbuf, RXBUF_SIZE);

    int rc = run_until_done(&ctx, "AT", NULL, 100,
                            NULL, 0, 50, 100);
    assert(rc == SIM7600_AT_TIMEOUT);
    printf("  timeout: PASS\n");
}

static void test_multi_line(void)
{
    sim7600_at_ctx_t ctx;
    uint8_t rxbuf[RXBUF_SIZE];
    char resp[128];

    mock_port_reset();
    sim7600_at_init(&ctx, mock_port_handle(), rxbuf, RXBUF_SIZE);

    sim7600_at_send_cmd(&ctx, "AT+CSQ", NULL, 1000, resp, sizeof(resp));
    sim7600_at_feed(&ctx, (const uint8_t *)"+CSQ: 15,99\r\n\r\nOK\r\n", 19);
    sim7600_at_task(&ctx);
    int rc = sim7600_at_send_cmd(&ctx, "AT+CSQ", NULL, 1000, resp, sizeof(resp));
    assert(rc == SIM7600_AT_OK);
    assert(strcmp(resp, "+CSQ: 15,99") == 0);
    printf("  multi_line: PASS\n");
}

static void test_urc_during_cmd(void)
{
    sim7600_at_ctx_t ctx;
    uint8_t rxbuf[RXBUF_SIZE];

    mock_port_reset();
    sim7600_at_init(&ctx, mock_port_handle(), rxbuf, RXBUF_SIZE);
    g_urc_count    = 0;
    g_last_urc[0]  = '\0';
    sim7600_at_register_urc(&ctx, "+CMQTTRXSTART", urc_handler, NULL);

    sim7600_at_send_cmd(&ctx, "AT+CMQTTSTART", NULL, 1000, NULL, 0);
    sim7600_at_feed(&ctx,
                    (const uint8_t *)"+CMQTTRXSTART: 0,5,11\r\nOK\r\n", 27);
    sim7600_at_task(&ctx);
    int rc = sim7600_at_send_cmd(&ctx, "AT+CMQTTSTART",
                                 NULL, 1000, NULL, 0);
    assert(rc == SIM7600_AT_OK);
    assert(g_urc_count == 1);
    assert(strcmp(g_last_urc, "+CMQTTRXSTART: 0,5,11") == 0);
    printf("  urc_during_cmd: PASS\n");
}

int main(void)
{
    printf("test_at:\n");
    test_ok();
    test_error();
    test_timeout();
    test_multi_line();
    test_urc_during_cmd();
    printf("test_at: ALL PASS\n");
    return 0;
}
