# Nucleo F412ZG echo example

The minimum firmware that proves the AT engine and the STM32 port can talk to a SIM7600G H. M2 milestone deliverable, no network or MQTT yet. The firmware sends `AT` every two seconds and prints the response on the ST Link Virtual COM Port at 115200 8N1. Expected output, once a minute or so after the modem boots, is a steady stream of `AT -> OK`.

## Hardware

Board: STMicroelectronics NUCLEO-F412ZG. Other STM32 boards should work after adjusting pins, clock, and DMA stream numbers in main.c (see port/stm32_hal/README.md for the per family CubeMX checklist).

Wiring between the Nucleo Morpho/Arduino headers and the SIM7600G H breakout:

| Nucleo pin | Arduino label | Breakout pin |
|---|---|---|
| PA2 | D1 (TX) | RXD |
| PA3 | D0 (RX) | TXD |
| PA0 | A0 | PWRKEY |
| PA1 | A1 | RESET |
| PA4 | A2 | STATUS |
| GND | GND | GND |
| (separate 2 A supply) |  | VBAT (do not power from the Nucleo 3V3 rail; the modem peaks above 1 A on TX) |

Tie the supply ground to the Nucleo ground.

## Build and flash

Two paths. Pick whichever you prefer.

### Path A: STM32CubeIDE

1. File, New, STM32 Project. Pick board NUCLEO F412ZG. Initialise all peripherals to default. Generate code.
2. Replace `Core/Src/main.c` with the file in this directory.
3. Add `Core/Inc/stm32_hal_glue.h` from this directory.
4. Add `core/` and `port/stm32_hal/` to the project's include paths and source paths. The simplest way is to copy them as linked folders, or add the source files directly.
5. Build (hammer icon).
6. Flash via the green run button. CubeIDE drives the on board ST Link.

### Path B: command line make

This path needs STM32CubeF4 (the ST firmware package) on disk and arm-none-eabi-gcc on PATH. A Makefile to wire those up will land in M8 alongside CI; until then CubeIDE is the supported build path.

## Expected output

Open a serial terminal at 115200 8N1 on the ST Link VCP (it appears as `/dev/tty.usbmodem*` on macOS or a `COMx` on Windows). After flashing, you should see:

```
sim7600 echo test starting
AT -> OK
AT -> OK
AT -> OK
...
```

If the first few lines say `AT -> rc=-1` (timeout) it is normal during the modem's boot window. The line stabilises to `AT -> OK` once the modem is fully up.

## What can go wrong

* `AT -> rc=-1` forever: check the wiring (TX/RX swap is the usual culprit), the 2 A supply, and that PWRKEY is being driven correctly for your breakout's polarity. Some breakouts invert PWRKEY through a transistor; if yours is direct, swap the polarities in `modem_power_on()`.
* No output on the VCP at all: confirm the serial terminal is on the ST Link CDC port (not the modem UART).
* `AT -> rc=-2`: the modem is responding but emitting ERROR. Likely AT echo is enabled; the engine treats lines literally. M2 does not configure the modem; this surfaces as a known quirk that goes away in M4 when we send `ATE0` early in init.
