# sim7600-aws-mqtt

A portable C library for using SIMCom SIM7600 series modems with AWS IoT Core MQTT, with platform ports for STM32 (generic HAL API) and Teensy 4.1 (Arduino).

## Mandatory rules

Ankit set these on 2026 May 01. They govern every reply, tool call, and commit on this project. They take precedence over anything else in this document. Existing prose written before commit 792f77b is grandfathered; everything authored from this commit forward must comply.

### Rule 0. No dashes

Never emit the hyphen character, the en dash, or the em dash in any prose, markdown, commit message, or code comment authored on this project. Use asterisks for markdown bullets. Use commas, the words "and", "to", "or", or a rephrase, in place of dash connectors. Hyphens that are part of a literal identifier (filename, repo slug, package name, AT command token, CLI flag, URL, SPDX tag) are unavoidable and must be reproduced verbatim when quoting.

### Rule 1. Think before coding

State assumptions explicitly. If uncertain, ask. If multiple interpretations exist, present them; do not pick silently. If a simpler approach exists, say so and push back when warranted. If something is unclear, stop, name what is confusing, and ask.

### Rule 2. Simplicity first

Write the minimum code that solves the problem. No speculative features. No abstractions for code that has only one caller. No flexibility or configurability that was not requested. No error handling for impossible scenarios. If 200 lines could be 50, rewrite. Ask: would a senior engineer call this overcomplicated? If yes, simplify.

### Rule 3. Surgical changes

Touch only what you must. Do not improve adjacent code, comments, or formatting. Do not refactor things that are not broken. Match existing style even if you would do it differently. If unrelated dead code is noticed, mention it; do not delete it. Remove only orphans that the current change created. Every changed line must trace directly to the user request.

### Rule 4. Goal driven execution

Define success criteria; loop until verified. Transform vague tasks into verifiable goals: "add validation" becomes "write tests for invalid inputs, then make them pass"; "fix the bug" becomes "write a test that reproduces it, then make it pass"; "refactor X" becomes "ensure tests pass before and after". For multi step tasks, state a brief plan with a verify step per item. Strong success criteria allow independent looping; weak criteria require constant clarification.

## Goals (v1.0)

- Connect SIM7600G-H to AWS IoT Core over native modem TLS.
- MQTT publish + subscribe with QoS 0 and QoS 1.
- Non-blocking, single-task (cooperatively scheduled via tick function).
- Clean separation between platform-agnostic core and platform ports.
- Production-quality: DMA UART against the generic STM32 HAL API, idle-line interrupt, no busy-waits in hot paths.

## Non-goals (v1.0 — do NOT implement)

- HTTP, FTP, SMS, voice, GNSS, FOTA.
- PPP / lwIP integration.
- mbedTLS on the host MCU. We use the modem's built-in TLS via AT+CSSL* commands.
- Multiple concurrent MQTT clients.
- Brokers other than AWS IoT Core (the code may work, but only AWS is tested and documented).
- QoS 2 (QoS 0 and QoS 1 only).
- SIMCom modems other than the SIM7600 series (no A76xx, no SIM7080, no SIM800).
- Quectel, u-blox, or other vendor modems.

The reference STM32 example targets STM32F412 (Nucleo-F412ZG). The port code in `port/stm32_hal/` must be written against the generic STM32 HAL API and compile cleanly against any STM32 family HAL (F0/F1/F3/F4/F7/G0/G4/H7/L0/L1/L4/L5/U5/WB/WL). Use `HAL_UARTEx_RxEventCallback` where available and provide a fallback path for older HALs gated on a feature macro. Do not include any family-specific `#ifdef STM32F4` (or other family) logic in the port. Tested boards are documented explicitly in the port README; other STM32 boards are documented as "should work, untested, PRs welcome."

The reference Teensy example targets Teensy 4.1. The port code should also compile and run on Teensy 4.0, 3.6, 3.5, and 3.2 without changes, but only Teensy 4.1 is tested in v1.0.

If you find yourself adding any of the non-goals above, STOP and ask.

## Architecture

```
core/                  Platform-agnostic C99 code
  sim7600_at.{h,c}     AT command engine (send, wait, URC dispatch)
  sim7600_net.{h,c}    Registration + APN + IP context
  sim7600_tls.{h,c}    Cert upload + SSL context config
  sim7600_mqtt.{h,c}   MQTT over CMQTT* AT commands
  sim7600_ringbuf.{h,c}  Single-producer/single-consumer ring buffer
  sim7600_port.h       Platform interface contract (header only)

port/stm32_hal/        Generic STM32 HAL implementation of sim7600_port.h (family-agnostic)
port/teensy/           Teensy 4.1 (Arduino) implementation, C++ shim with extern "C"
```

The core MUST compile as C99 with no platform headers. It MUST NOT include `stm32xxx_hal.h`, `Arduino.h`, or any other platform header. The only seam is `sim7600_port.h`.

## Coding standards

- C99 for core. C++ only allowed in `port/teensy/`.
- Indent: 4 spaces, no tabs.
- Naming: `sim7600_<module>_<verb>()` for public functions. Static functions get no prefix.
- All public types prefixed with `sim7600_` and `_t` suffix.
- All return codes are `int`, with 0 for success and negative `SIM7600_ERR_*` for failure. Define error codes in `sim7600_at.h`.
- No `malloc` in core. All buffers are caller-provided or static with compile-time size.
- No floating point.
- All public APIs documented with Doxygen-style comments in headers.
- No platform-specific code in core. No `#ifdef STM32` in core files. Ever.

## File headers

Every source file (`.c`, `.h`, `.cpp`, `.ino`) MUST begin with this exact header, with no variation:

```c
/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */
```

Markdown files, JSON, YAML, Makefiles, and `.ioc` files do NOT get a header. Only source code.

Update the GitHub URL once the repo is pushed. Do not invent a URL — leave the placeholder above as-is until the user confirms the canonical URL.

## State machine discipline

The core is non-blocking. The application calls `sim7600_task(&ctx)` periodically (e.g. every 10 ms or in main loop). All state machines (network registration, MQTT connect, publish flow) advance inside `sim7600_task`. No function in core may busy-wait longer than 10 ms.

The only exception: `sim7600_port_delay_ms` may be called for hardware power sequencing (e.g. PWRKEY hold time), and only from `sim7600_init`/`sim7600_power_on` paths.

## AT engine rules

- Every AT command has a timeout. No infinite waits.
- URCs (unsolicited result codes like `+CMQTTRXSTART`) are dispatched via callbacks registered at init.
- The AT parser reads from a ring buffer fed by the platform's UART RX path (DMA + idle-line via the generic STM32 HAL API on STM32, ISR on Teensy).
- Line-based parsing: split on `\r\n`, trim, dispatch.
- Reference: SIM7600 Series AT Command Manual is at `docs/reference/SIM7600_AT_Manual.pdf` (already downloaded). Consult it for exact command syntax, response formats, and timing requirements before writing any AT command sequence. Do not guess.

## Commit conventions

- Conventional Commits: feat:, fix:, docs:, refactor:, test:, chore:.
- Never add "Co-Authored-By" trailers.
- Never add "Generated with Claude Code" or similar attribution.
- No emojis in commit messages.
- Keep messages factual and concise.

## Build verification

- Core must compile cleanly with `arm-none-eabi-gcc -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter`.
- The STM32 port (`port/stm32_hal/`) must compile cleanly against the generic STM32 HAL API for at least two families in CI (the F412 reference plus one other, e.g. G4 or H7) using each family's CMSIS + HAL headers. This proves the port is family-agnostic and not silently coupled to F4.
- STM32 reference example (Nucleo-F412ZG) must build with the included Makefile or CubeIDE project.
- Teensy example must build with `arduino-cli compile --fqbn teensy:avr:teensy41`.
- A GitHub Actions workflow (`.github/workflows/ci.yml`) runs all of the above on push.

## Testing approach

- No unit test framework dependency in core for v1.0 — too much overhead for the time budget.
- Each module has a corresponding `tests/test_<module>.c` host-side test compiled with native gcc, using a mocked port. These run in CI.
- Hardware-in-the-loop testing is manual via the example projects.

## What to do when you get stuck

- If a SIM7600 AT response format is unclear, consult `docs/reference/SIM7600_AT_Manual.pdf`. Cite the page number in code comments.
- If platform behaviour is unclear, write the smallest possible test sketch and ask the user to run it on hardware.
- Never invent AT commands or response formats. If unsure, stop and ask.

## What NOT to do

- Do not commit binaries, build artefacts, .pioenvs, .vscode, build/, Debug/, Release/.
- Do not add dependencies (no CMSIS-DSP, no FreeRTOS, no mbedTLS, no Arduino libraries beyond Teensy core).
- Do not refactor the core to be C++.
- Do not add features outside the v1.0 scope above without explicit approval.
- Do not add files to the root unless explicitly listed in this document.
