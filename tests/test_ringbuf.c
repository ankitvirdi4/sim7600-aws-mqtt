/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#include "sim7600_ringbuf.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_init_pow2(void)
{
    uint8_t buf[16];
    sim7600_ringbuf_t rb;
    assert(sim7600_ringbuf_init(&rb, buf, 16) == 0);
    assert(sim7600_ringbuf_init(&rb, buf, 17) == -1);
    assert(sim7600_ringbuf_init(&rb, buf, 0)  == -1);
    printf("  init_pow2: PASS\n");
}

static void test_empty(void)
{
    uint8_t buf[16];
    sim7600_ringbuf_t rb;
    sim7600_ringbuf_init(&rb, buf, 16);
    assert(sim7600_ringbuf_available(&rb)  == 0);
    assert(sim7600_ringbuf_free_space(&rb) == 16);

    uint8_t out;
    assert(sim7600_ringbuf_read(&rb, &out, 1) == 0);
    printf("  empty: PASS\n");
}

static void test_write_read(void)
{
    uint8_t buf[16];
    sim7600_ringbuf_t rb;
    sim7600_ringbuf_init(&rb, buf, 16);

    uint8_t in[5] = { 1, 2, 3, 4, 5 };
    assert(sim7600_ringbuf_write(&rb, in, 5) == 5);
    assert(sim7600_ringbuf_available(&rb) == 5);

    uint8_t out[5];
    assert(sim7600_ringbuf_read(&rb, out, 5) == 5);
    assert(memcmp(in, out, 5) == 0);
    assert(sim7600_ringbuf_available(&rb) == 0);
    printf("  write_read: PASS\n");
}

static void test_wraparound(void)
{
    uint8_t buf[8];
    sim7600_ringbuf_t rb;
    sim7600_ringbuf_init(&rb, buf, 8);

    uint8_t a[5] = { 1, 2, 3, 4, 5 };
    sim7600_ringbuf_write(&rb, a, 5);

    uint8_t out[5];
    sim7600_ringbuf_read(&rb, out, 5);
    /* head=5 tail=5; the next 6 byte write wraps. */
    uint8_t b[6] = { 9, 8, 7, 6, 5, 4 };
    assert(sim7600_ringbuf_write(&rb, b, 6) == 6);
    assert(sim7600_ringbuf_available(&rb) == 6);

    uint8_t out2[6];
    assert(sim7600_ringbuf_read(&rb, out2, 6) == 6);
    assert(memcmp(b, out2, 6) == 0);
    printf("  wraparound: PASS\n");
}

static void test_full(void)
{
    uint8_t buf[8];
    sim7600_ringbuf_t rb;
    sim7600_ringbuf_init(&rb, buf, 8);

    uint8_t a[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    assert(sim7600_ringbuf_write(&rb, a, 8) == 8);
    assert(sim7600_ringbuf_free_space(&rb) == 0);
    assert(sim7600_ringbuf_write(&rb, (uint8_t *)"x", 1) == 0);

    uint8_t a2[3] = { 9, 9, 9 };
    assert(sim7600_ringbuf_write(&rb, a2, 3) == 0);

    uint8_t out[3];
    sim7600_ringbuf_read(&rb, out, 3);
    assert(sim7600_ringbuf_write(&rb, a2, 3) == 3);
    printf("  full: PASS\n");
}

int main(void)
{
    printf("test_ringbuf:\n");
    test_init_pow2();
    test_empty();
    test_write_read();
    test_wraparound();
    test_full();
    printf("test_ringbuf: ALL PASS\n");
    return 0;
}
