# Teensy 4.1 echo example

The minimum sketch that proves the AT engine and the Teensy port can talk to a SIM7600G H. M3 milestone deliverable, no network or MQTT yet. The sketch sends `AT` every two seconds and prints the response on USB Serial at 115200 8N1. Expected output, once the modem boots, is a steady stream of `AT -> OK`.

## Hardware

Board: PJRC Teensy 4.1. Other Teensy boards (4.0, 3.6, 3.5, 3.2) should compile and run after adjusting the pin numbers in the sketch, but only Teensy 4.1 is tested in v0.1.0.

Wiring between the Teensy 4.1 and the SIM7600G H breakout:

| Teensy pin | Function | Breakout pin |
|---|---|---|
| 1 | Serial1 TX | RXD |
| 0 | Serial1 RX | TXD |
| 2 | PWRKEY drive | PWRKEY |
| 3 | RESET drive | RESET |
| 4 | STATUS sense | STATUS |
| GND | ground | GND |
| (separate 2 A supply) | | VBAT (do not power from USB; the modem peaks above 1 A on TX) |

Tie the supply ground to the Teensy ground.

## Build

This repo is structured as an Arduino library: a `library.properties` at the repo root and a `src/` directory that links to the core and Teensy port sources. arduino-cli and the Arduino IDE both pick it up.

### Path A: arduino-cli (recommended)

```
brew install arduino-cli
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://www.pjrc.com/teensy/package_teensy_index.json
arduino-cli core update-index
arduino-cli core install teensy:avr

arduino-cli compile \
    --fqbn teensy:avr:teensy41 \
    --library /Users/ankit/Documents/SIMCOM7600 \
    examples/teensy41_aws_publisher
```

The `--library` flag points at the repo root (where `library.properties` lives). For your machine, replace the path accordingly.

### Path B: Arduino IDE with Teensyduino

1. Install Arduino IDE 1.8 or 2.x and the Teensyduino addon from PJRC.
2. Symlink (or copy) the repo into your sketchbook libraries folder, typically `~/Documents/Arduino/libraries/sim7600-aws-mqtt`.
3. Open `examples/teensy41_aws_publisher/teensy41_aws_publisher.ino` in the IDE.
4. Tools > Board > Teensy 4.1, Tools > USB Type > Serial.
5. Click Upload.

## Flash

`arduino-cli upload` or the Teensy Loader GUI both work. The Teensy enters bootloader mode automatically when the IDE or arduino-cli initiates an upload.

## Expected output

Open a serial terminal at 115200 8N1 on the Teensy USB Serial port. After flashing, you should see:

```
sim7600 echo test starting
AT -> OK
AT -> OK
...
```

`AT -> rc=-1` for the first few iterations is normal during the modem's boot window.

## What can go wrong

* `AT -> rc=-1` forever: check Serial1 wiring (TX RX swap is the usual culprit), the 2 A supply, and PWRKEY polarity for your breakout. Some breakouts invert PWRKEY through a transistor; if yours is direct, swap the polarities in `setup()`.
* No output on USB Serial at all: confirm Tools > USB Type is set to Serial in Arduino IDE, or `--build-property build.usb_type=USB_SERIAL` is implied by your fqbn (it is for `teensy41`).
* `AT -> rc=-2`: the modem is responding but emitting ERROR. Likely AT echo is enabled. M3 does not configure the modem; this surfaces as a known quirk that is fixed in M4 when init sends `ATE0`.
* RX buffer overflow on long bursts: Teensy HardwareSerial has a software RX buffer of around 64 bytes. The polling design in port/teensy is sufficient for AT command pacing but can drop bytes during high throughput phases (cert upload at M5). A v0.2.0 port revision can move RX to LPUART DMA on the IMXRT1062 to remove this limit.
