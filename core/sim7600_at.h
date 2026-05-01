/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#ifndef SIM7600_AT_H
#define SIM7600_AT_H

#include <stddef.h>
#include <stdint.h>
#include "sim7600_port.h"
#include "sim7600_ringbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes. */
#define SIM7600_AT_OK       0
#define SIM7600_AT_PENDING  1
#define SIM7600_AT_TIMEOUT  (-1)
#define SIM7600_AT_ERROR    (-2)
#define SIM7600_AT_BUSY     (-3)
#define SIM7600_AT_INVAL    (-4)

#ifndef SIM7600_AT_URC_TABLE_SIZE
#define SIM7600_AT_URC_TABLE_SIZE 8
#endif

#ifndef SIM7600_AT_LINE_BUF_SIZE
#define SIM7600_AT_LINE_BUF_SIZE 256
#endif

/** URC line callback. line is null terminated, valid only during the call. */
typedef void (*sim7600_at_urc_cb_t)(const char *line, void *user);

typedef struct {
    const char          *prefix;
    sim7600_at_urc_cb_t  cb;
    void                *user;
} sim7600_at_urc_entry_t;

typedef struct {
    sim7600_port_t   *port;
    sim7600_ringbuf_t rb;

    /* In flight command state. */
    int         cmd_in_flight;
    const char *expected;
    char       *response_out;
    size_t      response_max;
    size_t      response_len;
    uint32_t    t_start;
    uint32_t    timeout_ms;
    int         last_status;

    /* Line accumulation buffer. */
    char   line_buf[SIM7600_AT_LINE_BUF_SIZE];
    size_t line_len;

    /* URC table. */
    sim7600_at_urc_entry_t urc[SIM7600_AT_URC_TABLE_SIZE];
    size_t                 urc_count;
} sim7600_at_ctx_t;

/**
 * @brief Initialise an AT engine context.
 * @param port  Platform port pointer. Must be non NULL.
 * @param rxbuf Caller provided RX ring buffer storage.
 * @param rxbuf_size Capacity in bytes; must be a power of two.
 * @return SIM7600_AT_OK on success, SIM7600_AT_INVAL on bad arguments.
 */
int sim7600_at_init(sim7600_at_ctx_t *ctx, sim7600_port_t *port,
                    uint8_t *rxbuf, size_t rxbuf_size);

/**
 * @brief Send an AT command and poll its result.
 *
 * The first call with no command in flight transmits the command and
 * returns SIM7600_AT_PENDING. Subsequent calls return SIM7600_AT_PENDING
 * until sim7600_at_task() drains a terminator line. Once the command
 * completes, this function returns SIM7600_AT_OK / SIM7600_AT_ERROR /
 * SIM7600_AT_TIMEOUT exactly once, and the next call begins a new command.
 *
 * @param expected Optional success line to match exactly. NULL means "OK".
 *                 Even when expected is set, a literal "OK" line still
 *                 terminates the command as success.
 * @param timeout_ms Maximum time to wait before giving up.
 * @param response_out Optional buffer to capture intermediate response
 *                     lines joined with '\n'. NULL to discard.
 * @param response_max Capacity of response_out.
 * @return SIM7600_AT_OK, SIM7600_AT_PENDING, SIM7600_AT_TIMEOUT,
 *         or SIM7600_AT_ERROR.
 */
int sim7600_at_send_cmd(sim7600_at_ctx_t *ctx, const char *cmd,
                        const char *expected, uint32_t timeout_ms,
                        char *response_out, size_t response_max);

/**
 * @brief Register a URC line handler.
 *
 * Lines starting with prefix are dispatched to cb and never delivered as a
 * command response. Returns SIM7600_AT_OK or SIM7600_AT_INVAL if the table
 * is full.
 */
int sim7600_at_register_urc(sim7600_at_ctx_t *ctx, const char *prefix,
                            sim7600_at_urc_cb_t cb, void *user);

/**
 * @brief Push received bytes from the platform RX path into the ring buffer.
 *
 * Called by the platform port from its DMA / idle line / serialEvent
 * callback (or from tests).
 */
void sim7600_at_feed(sim7600_at_ctx_t *ctx, const uint8_t *bytes, size_t len);

/**
 * @brief Advance the AT engine state machine.
 *
 * Drains the ring buffer, accumulates lines, dispatches URCs, and detects
 * command terminators and timeouts. Must be called periodically by the
 * application (typically in the main loop).
 */
void sim7600_at_task(sim7600_at_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SIM7600_AT_H */
