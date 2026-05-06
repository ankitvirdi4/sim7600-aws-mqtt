# sim7600-aws-mqtt

A portable C library for using SIMCom SIM7600 cellular modems with AWS IoT Core MQTT over the modem's native TLS stack. Two platform ports ship in the same repo: a generic STM32 HAL port (reference example on a Nucleo-F412ZG, family agnostic) and a Teensy 4.1 (Arduino) port.

## Status

Pre release, version 0.0.1. The path to v0.1.0 is tracked in the Roadmap below.

What works today, locally verified:

* Platform agnostic AT command engine in C99 with host unit tests covering OK, ERROR, timeout, multi line response, and URC interleaved with a pending command.
* Generic STM32 HAL port, family agnostic. Freestanding subset of the core compiles cleanly under arm-none-eabi-gcc.
* Teensy 4.1 (Arduino) port. arduino-cli compile of the example sketch is clean.

Pending hardware bench verification:

* STM32 example on a Nucleo-F412ZG talks to a SIM7600G-H and prints `AT -> OK` on the ST-Link VCP at 115200.
* Teensy 4.1 example talks to a SIM7600G-H and prints `AT -> OK` on USB Serial at 115200.

Not yet implemented (path to v0.1.0):

* Cellular network registration plus APN bring up (M4).
* Native modem TLS plus AWS root CA, client cert, and key provisioning (M5).
* MQTT publish, subscribe, and exponential backoff reconnect (M6).
* End to end AWS IoT publisher example on both platforms (M7).
* CI plus the v0.1.0 release tag (M8).

## Architecture

```
core/                  Platform agnostic C99
  sim7600_at           AT command engine, URC dispatch, line based parser
  sim7600_ringbuf      Single producer single consumer byte ring buffer
  sim7600_port.h       Platform interface contract (the only seam)

port/stm32_hal/        Generic STM32 HAL implementation, family agnostic
port/teensy/           Teensy 4.1 (Arduino) implementation, C++ shim with extern "C"

examples/              One example per platform
tests/                 Host gcc unit tests with a mock port
src/                   Arduino library entry; symlinks into core and port/teensy
```

The core compiles as C99 with no platform headers. It does not include any STM32 HAL header, any Arduino header, or any other platform header. The only seam is `sim7600_port.h`. The same core links unmodified against both ports.

## Quick start

### Teensy 4.1

```
brew install arduino-cli
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://www.pjrc.com/teensy/package_teensy_index.json
arduino-cli core update-index
arduino-cli core install teensy:avr

git clone https://github.com/ankitvirdi4/sim7600-aws-mqtt
cd sim7600-aws-mqtt
arduino-cli compile --fqbn teensy:avr:teensy41 --library . examples/teensy41_aws_publisher
arduino-cli upload  --fqbn teensy:avr:teensy41 --port /dev/cu.usbmodem* examples/teensy41_aws_publisher
```

Wiring, USB type, and serial expectations are in [examples/teensy41_aws_publisher/README.md](examples/teensy41_aws_publisher/README.md).

On Apple Silicon Macs, the Teensy core ships x86_64 binaries that need Rosetta 2: `softwareupdate --install-rosetta --agree-to-license`.

### STM32 (Nucleo-F412ZG via STM32CubeIDE)

1. STM32CubeIDE: New STM32 Project, Board Selector, NUCLEO-F412ZG, accept defaults.
2. Replace `Core/Src/main.c` with [examples/stm32f412_nucleo_aws_publisher/Core/Src/main.c](examples/stm32f412_nucleo_aws_publisher/Core/Src/main.c).
3. Add [examples/stm32f412_nucleo_aws_publisher/Core/Inc/stm32_hal_glue.h](examples/stm32f412_nucleo_aws_publisher/Core/Inc/stm32_hal_glue.h).
4. Add `core/` and `port/stm32_hal/` to the project's include and source paths.
5. Build, flash, observe USART3 (the ST-Link VCP) at 115200 8N1.

Wiring, CubeMX configuration, and troubleshooting are in [examples/stm32f412_nucleo_aws_publisher/README.md](examples/stm32f412_nucleo_aws_publisher/README.md). Other STM32 families work with the same port code; the only family decision is one `#include` in `stm32_hal_glue.h`.

## Roadmap

The library is being built milestone by milestone, with hardware verification at each step.

* M1, done: AT engine plus host tests.
* M2, code complete, hardware verification pending: STM32 port plus Nucleo-F412ZG echo example.
* M3, code complete, hardware verification pending: Teensy 4.1 port plus echo sketch.
* M4, next: cellular network registration plus APN bring up. Three UK as the reference carrier.
* M5: native modem TLS via AT+CSSLCFG and AT+CCERTDOWN; AWS root CA, client cert, and private key uploaded to modem flash.
* M6: MQTT publish, subscribe, and exponential backoff reconnect via AT+CMQTT*.
* M7: end to end AWS IoT publisher on both platforms, telemetry every 10 seconds.
* M8: GitHub Actions CI (host tests, multi family STM32 compile, Teensy compile) plus the v0.1.0 release tag.
* M9: announcement on Medium, Hackster, and r/embedded.

QoS 1 ships in v0.1.0 if M6 stays inside its time budget; otherwise it slips to v0.2.0 and the limitation is documented.

## Repo layout

* `core/` and `port/` are the library proper.
* `examples/` has one example project per platform.
* `tests/` has host side unit tests with a mock port and a Makefile that runs them under host gcc and an arm cross compile sanity check.
* `src/` is the Arduino library entry, populated by symlinks into `core/` and `port/teensy/`. The same code is the canonical source; the symlinks just expose the Teensy slice in the layout `arduino-cli` and the Arduino IDE expect.
* `docs/reference/` holds the SIM7600 AT command manual that the higher milestones will reference.
* `CLAUDE.md` is the working spec and house rules; read it first if you plan to contribute.

## License

MIT, copyright (c) 2026 Ankit Virdi. See [LICENSE](LICENSE).

## Contributing

Issues and PRs welcome. The easiest contribution is hardware diversity: an additional STM32 board (any family covered by the generic HAL port), an additional Teensy variant, or a wiring photo for a board that is not yet documented. The next easiest is documentation; if a step in either example README is unclear or wrong, a short PR with a fix is the right answer.

For source changes please follow the conventions in [CLAUDE.md](CLAUDE.md).
