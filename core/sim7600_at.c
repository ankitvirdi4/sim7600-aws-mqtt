/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#include "sim7600_at.h"

#include <string.h>

static void process_line(sim7600_at_ctx_t *ctx, const char *line);
static int  line_starts_with(const char *line, const char *prefix);
static void append_response(sim7600_at_ctx_t *ctx, const char *line);

int sim7600_at_init(sim7600_at_ctx_t *ctx, sim7600_port_t *port,
                    uint8_t *rxbuf, size_t rxbuf_size)
{
    if (ctx == NULL) return SIM7600_AT_INVAL;
    int rc = sim7600_ringbuf_init(&ctx->rb, rxbuf, rxbuf_size);
    if (rc != 0) return SIM7600_AT_INVAL;

    ctx->port          = port;
    ctx->cmd_in_flight = 0;
    ctx->expected      = NULL;
    ctx->response_out  = NULL;
    ctx->response_max  = 0;
    ctx->response_len  = 0;
    ctx->t_start       = 0;
    ctx->timeout_ms    = 0;
    ctx->last_status   = SIM7600_AT_OK;
    ctx->line_len      = 0;
    ctx->urc_count     = 0;
    return SIM7600_AT_OK;
}

int sim7600_at_send_cmd(sim7600_at_ctx_t *ctx, const char *cmd,
                        const char *expected, uint32_t timeout_ms,
                        char *response_out, size_t response_max)
{
    if (!ctx->cmd_in_flight) {
        /* Start a new command. */
        sim7600_port_uart_write(ctx->port,
                                (const uint8_t *)cmd, strlen(cmd));
        sim7600_port_uart_write(ctx->port,
                                (const uint8_t *)"\r\n", 2);
        ctx->expected     = expected;
        ctx->response_out = response_out;
        ctx->response_max = response_max;
        ctx->response_len = 0;
        if (response_out != NULL && response_max > 0) {
            response_out[0] = '\0';
        }
        ctx->t_start       = sim7600_port_millis();
        ctx->timeout_ms    = timeout_ms;
        ctx->last_status   = SIM7600_AT_PENDING;
        ctx->cmd_in_flight = 1;
        return SIM7600_AT_PENDING;
    }

    /* Poll. */
    if (ctx->last_status == SIM7600_AT_PENDING) {
        return SIM7600_AT_PENDING;
    }

    /* Terminal status reached; deliver once and clear. */
    int s = ctx->last_status;
    ctx->cmd_in_flight = 0;
    return s;
}

int sim7600_at_register_urc(sim7600_at_ctx_t *ctx, const char *prefix,
                            sim7600_at_urc_cb_t cb, void *user)
{
    if (ctx->urc_count >= SIM7600_AT_URC_TABLE_SIZE) {
        return SIM7600_AT_INVAL;
    }
    ctx->urc[ctx->urc_count].prefix = prefix;
    ctx->urc[ctx->urc_count].cb     = cb;
    ctx->urc[ctx->urc_count].user   = user;
    ctx->urc_count++;
    return SIM7600_AT_OK;
}

void sim7600_at_feed(sim7600_at_ctx_t *ctx, const uint8_t *bytes, size_t len)
{
    sim7600_ringbuf_write(&ctx->rb, bytes, len);
}

void sim7600_at_task(sim7600_at_ctx_t *ctx)
{
    /* Drain the ring buffer one byte at a time, accumulate lines. */
    uint8_t b;
    while (sim7600_ringbuf_read(&ctx->rb, &b, 1) == 1) {
        if (b == '\r') {
            continue;
        }
        if (b == '\n') {
            if (ctx->line_len > 0) {
                ctx->line_buf[ctx->line_len] = '\0';
                process_line(ctx, ctx->line_buf);
                ctx->line_len = 0;
            }
            continue;
        }
        if (ctx->line_len < SIM7600_AT_LINE_BUF_SIZE - 1) {
            ctx->line_buf[ctx->line_len++] = (char)b;
        }
        /* Overflow: leftover bytes are dropped until the next \n. */
    }

    /* Timeout check on the in flight command. */
    if (ctx->cmd_in_flight && ctx->last_status == SIM7600_AT_PENDING) {
        uint32_t now = sim7600_port_millis();
        if ((uint32_t)(now - ctx->t_start) > ctx->timeout_ms) {
            ctx->last_status = SIM7600_AT_TIMEOUT;
        }
    }
}

static int line_starts_with(const char *line, const char *prefix)
{
    return strncmp(line, prefix, strlen(prefix)) == 0;
}

static void append_response(sim7600_at_ctx_t *ctx, const char *line)
{
    if (ctx->response_out == NULL || ctx->response_max == 0) {
        return;
    }
    size_t l    = strlen(line);
    size_t need = ctx->response_len
                + (ctx->response_len > 0 ? 1 : 0)
                + l + 1;
    if (need > ctx->response_max) {
        return;
    }
    if (ctx->response_len > 0) {
        ctx->response_out[ctx->response_len++] = '\n';
    }
    memcpy(ctx->response_out + ctx->response_len, line, l);
    ctx->response_len += l;
    ctx->response_out[ctx->response_len] = '\0';
}

static void process_line(sim7600_at_ctx_t *ctx, const char *line)
{
    /* URC takes precedence over command response handling. */
    for (size_t i = 0; i < ctx->urc_count; i++) {
        if (line_starts_with(line, ctx->urc[i].prefix)) {
            ctx->urc[i].cb(line, ctx->urc[i].user);
            return;
        }
    }

    /* No command in flight or already concluded: ignore. */
    if (!ctx->cmd_in_flight || ctx->last_status != SIM7600_AT_PENDING) {
        return;
    }

    /* Custom expected success line. */
    if (ctx->expected != NULL && strcmp(line, ctx->expected) == 0) {
        ctx->last_status = SIM7600_AT_OK;
        return;
    }

    /* Default success terminator. */
    if (strcmp(line, "OK") == 0) {
        ctx->last_status = SIM7600_AT_OK;
        return;
    }

    /* Error terminators. */
    if (strcmp(line, "ERROR") == 0
        || line_starts_with(line, "+CME ERROR")
        || line_starts_with(line, "+CMS ERROR")) {
        ctx->last_status = SIM7600_AT_ERROR;
        return;
    }

    /* Intermediate response line; capture if a buffer was provided. */
    append_response(ctx, line);
}
