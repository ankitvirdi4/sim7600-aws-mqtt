# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Ankit Virdi

CC      ?= gcc
ARMCC   ?= arm-none-eabi-gcc
CFLAGS  := -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter -I../core -I.
LDFLAGS :=

CORE_RINGBUF_SRC := ../core/sim7600_ringbuf.c
CORE_AT_SRC      := ../core/sim7600_ringbuf.c ../core/sim7600_at.c
CORE_ALL_SRC     := $(CORE_AT_SRC)

.PHONY: all test arm-check clean

all: test_ringbuf test_at

test: all
	@echo "running test_ringbuf"
	./test_ringbuf
	@echo "running test_at"
	./test_at
	@echo "all tests passed"

test_ringbuf: test_ringbuf.c $(CORE_RINGBUF_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_at: test_at.c mock_port.c $(CORE_AT_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# arm-check verifies the freestanding subset of core/ compiles under
# arm-none-eabi-gcc. sim7600_at.c uses string.h (strlen, strncmp,
# strcmp, memcpy) which requires a C library (newlib or picolibc).
# The bare arm-none-eabi-gcc formula on Homebrew does not bundle one,
# so we exclude sim7600_at.c here and let the STM32 example build in
# M2 (which links newlib-nano via CubeIDE) cover it.
ARM_FREESTANDING_SRC := \
    ../core/sim7600_port.h \
    ../core/sim7600_ringbuf.h \
    ../core/sim7600_at.h \
    ../core/sim7600_ringbuf.c

arm-check:
	@if command -v $(ARMCC) >/dev/null 2>&1; then \
	    echo "running arm cross compile check (freestanding subset)" && \
	    $(ARMCC) -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter \
	        -ffreestanding -I../core -fsyntax-only $(ARM_FREESTANDING_SRC) && \
	    echo "arm cross compile (freestanding subset) OK"; \
	else \
	    echo "$(ARMCC) not installed; skipping cross compile check"; \
	fi

clean:
	rm -f test_ringbuf test_at *.o
