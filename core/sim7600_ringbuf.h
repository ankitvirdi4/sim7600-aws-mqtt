/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#ifndef SIM7600_RINGBUF_H
#define SIM7600_RINGBUF_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Single producer single consumer byte ring buffer.
 *
 * Capacity must be a power of two. The producer is typically the platform
 * UART RX path (DMA callback or ISR). The consumer is the AT engine task.
 * head and tail are monotonically increasing; index by (head & mask).
 */
typedef struct {
    uint8_t        *buf;
    size_t          mask;
    volatile size_t head;
    volatile size_t tail;
} sim7600_ringbuf_t;

/**
 * @return 0 on success, -1 if buf is NULL or capacity is not a power of two.
 */
int sim7600_ringbuf_init(sim7600_ringbuf_t *rb, uint8_t *buf, size_t capacity);

/** @return number of bytes actually written (capped by free space). */
size_t sim7600_ringbuf_write(sim7600_ringbuf_t *rb, const uint8_t *src, size_t n);

/** @return number of bytes actually read (capped by available bytes). */
size_t sim7600_ringbuf_read(sim7600_ringbuf_t *rb, uint8_t *dst, size_t n);

size_t sim7600_ringbuf_available(const sim7600_ringbuf_t *rb);
size_t sim7600_ringbuf_free_space(const sim7600_ringbuf_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* SIM7600_RINGBUF_H */
