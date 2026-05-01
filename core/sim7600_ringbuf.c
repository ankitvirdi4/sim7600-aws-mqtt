/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#include "sim7600_ringbuf.h"

static int is_pow2(size_t n)
{
    return n != 0 && (n & (n - 1)) == 0;
}

int sim7600_ringbuf_init(sim7600_ringbuf_t *rb, uint8_t *buf, size_t capacity)
{
    if (rb == NULL || buf == NULL || !is_pow2(capacity)) {
        return -1;
    }
    rb->buf  = buf;
    rb->mask = capacity - 1;
    rb->head = 0;
    rb->tail = 0;
    return 0;
}

size_t sim7600_ringbuf_available(const sim7600_ringbuf_t *rb)
{
    return rb->head - rb->tail;
}

size_t sim7600_ringbuf_free_space(const sim7600_ringbuf_t *rb)
{
    return (rb->mask + 1) - sim7600_ringbuf_available(rb);
}

size_t sim7600_ringbuf_write(sim7600_ringbuf_t *rb, const uint8_t *src, size_t n)
{
    size_t free_space = sim7600_ringbuf_free_space(rb);
    size_t to_write   = (n < free_space) ? n : free_space;
    for (size_t i = 0; i < to_write; i++) {
        rb->buf[(rb->head + i) & rb->mask] = src[i];
    }
    rb->head += to_write;
    return to_write;
}

size_t sim7600_ringbuf_read(sim7600_ringbuf_t *rb, uint8_t *dst, size_t n)
{
    size_t available = sim7600_ringbuf_available(rb);
    size_t to_read   = (n < available) ? n : available;
    for (size_t i = 0; i < to_read; i++) {
        dst[i] = rb->buf[(rb->tail + i) & rb->mask];
    }
    rb->tail += to_read;
    return to_read;
}
