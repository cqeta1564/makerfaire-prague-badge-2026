# Maker Badge 2019 Game - PlatformIO

English README. Czech version: [README.cs.md](README.cs.md).

PlatformIO firmware for running a Maker Faire Prague 2019 style IR badge game
on the ESP32-S2 Czech Maker Badge.

The firmware lets badges exchange IDs over IR, store seen badges in NVS, show
the badge/team code on the e-paper display and onboard RGB LEDs, and then enter
deep sleep to save battery. The ESP wakes only by reset or power cycling in the
normal low-power flow.

## What It Does

- Pairs with another badge over the 2019 IR protocol
- Saves newly seen badge IDs in ESP32-S2 Preferences/NVS
- Shows your own ID, team ID, and seen IDs on the e-paper display
- Uses the four onboard NeoPixel LEDs as a visible ID/code indicator
- Responds to dump requests for gateway/debug tooling
- Supports Serial Monitor commands for setup, testing, dumping, and sleep
- Powers the added IR receiver from the existing LED/NeoPixel switched rail
- Enters deep sleep after actions and after an idle timeout

## Project Layout

| Path | Purpose |
| --- | --- |
| `platformio.ini` | PlatformIO environments and build flags |
| `include/BadgeConfig.h` | Pins, storage constants, sleep settings |
| `src/main.cpp` | Arduino `setup()` / `loop()` entry point |
| `src/GameLogic.*` | Pairing, showing, dump flow, idle timeout |
| `src/IrProtocol.*` | IR encode/decode/send/receive |
| `src/BadgeDisplay.*` | E-paper and NeoPixel output |
| `src/BadgePower.*` | Final low-power deep sleep sequence |
| `src/BadgeStorage.*` | Persistent ID/team/seen storage |
| `src/SerialCommands.*` | Serial Monitor command interface |

## Hardware

The base Maker Badge board does not include the old 2019 IR hardware. Add a
short-range IR LED and a 38 kHz IR receiver.

Default pins are defined in `include/BadgeConfig.h`:

| Function | Default |
| --- | --- |
| IR transmit LED | GPIO 11 (`PIN_IR_TX`) |
| IR receiver OUT | GPIO 12 (`PIN_IR_RX`) |
| IR receiver VCC | switched LED/NeoPixel `V+` rail |
| LED/NeoPixel power switch | GPIO 21 (`PIN_NEOPIXEL_PWR`) |
| NeoPixel data | GPIO 18 (`PIN_RGB_LED`) |
| E-paper power switch | GPIO 16 (`PIN_EPD_POWER`) |
| BOOT button | GPIO 0 |
| Touch show button | GPIO 1 |

## IR Wiring

Connect the added IR parts like this:

| IR part | Connect to |
| --- | --- |
| IR LED anode | GPIO 11 through a current-limiting resistor |
| IR LED cathode | GND |
| 38 kHz receiver OUT | GPIO 12 |
| 38 kHz receiver VCC | switched LED/NeoPixel `V+` rail |
| 38 kHz receiver GND | GND |

Do not connect the receiver VCC to permanent `3V3` if low sleep current matters.
The firmware expects the receiver to share the existing LED/NeoPixel power
switch. That rail is active-low: `LOW` means powered, `HIGH` means off.

## Board Revisions

| Board revision | LED / NeoPixel power rail | E-paper power switch | IR receiver VCC |
| --- | --- | --- | --- |
| rev. A | GPIO 21 (`PIN_NEOPIXEL_PWR`) | no dedicated GPIO power switch | LED/NeoPixel `V+` |
| rev. B | GPIO 21 (`PIN_NEOPIXEL_PWR`) | no dedicated GPIO power switch | LED/NeoPixel `V+` |
| rev. C | GPIO 21 (`PIN_NEOPIXEL_PWR`) | no dedicated GPIO power switch | LED/NeoPixel `V+` |
| rev. D | GPIO 21 (`PIN_NEOPIXEL_PWR`) | GPIO 16 (`PIN_EPD_POWER`) | LED/NeoPixel `V+` |

For rev. A badges with the older UC8151D / GDEW0213T5D display, enable
`MAKER_BADGE_REV_A_DISPLAY`.

## Power Behavior

The release firmware is aggressive about battery saving:

- after pairing, showing, dump completion, timeout, or serial `S`, the badge
  enters final deep sleep
- after `IDLE_SLEEP_TIMEOUT_MS` on the ready screen, it also enters deep sleep
- wake is intentionally reset-only; no button/touch wake source is enabled
- before sleep, the firmware turns off NeoPixels, the LED/IR power rail, the
  e-paper power rail, IR interrupt handling, and RTC sleep domains where
  supported

Important settings in `include/BadgeConfig.h`:

| Setting | Default | Meaning |
| --- | --- | --- |
| `SLEEP_AFTER_ACTION` | `1` | Sleep immediately after completed actions |
| `IDLE_SLEEP_TIMEOUT_MS` | `60000UL` | Ready-screen timeout before sleep |
| `PIN_IR_RX_POWER` | `PIN_NEOPIXEL_PWR` | IR receiver power rail |
| `IR_RX_POWER_ACTIVE_LOW` | `1` | `LOW` powers the IR receiver rail |

Set `SLEEP_AFTER_ACTION=0` while debugging if you do not want the badge to sleep
after every normal action. Set `IDLE_SLEEP_TIMEOUT_MS=0` to disable idle sleep.

## PlatformIO Environments

| Environment | Use |
| --- | --- |
| `maker_badge_esp32s2` | Normal release build |
| `maker_badge_debug` | Debug build with `SLEEP_AFTER_ACTION=0` and 15 s idle timeout |

The debug environment is useful when working through Serial Monitor because the
badge returns to the ready screen after normal actions instead of immediately
sleeping.

## Build

From this folder:

```powershell
python -m platformio run -e maker_badge_esp32s2
```

Debug build:

```powershell
python -m platformio run -e maker_badge_debug
```

If `pio` is available on `PATH`, the equivalent command is:

```powershell
pio run -e maker_badge_esp32s2
```

## Upload

Connect the badge over USB-C, turn it on, and run:

```powershell
python -m platformio run -e maker_badge_esp32s2 -t upload
```

Debug firmware upload:

```powershell
python -m platformio run -e maker_badge_debug -t upload
```

If PlatformIO does not auto-detect the serial port, add `upload_port` to
`platformio.ini` or pass it on the command line.

## Serial Monitor

Open the monitor at 115200 baud:

```powershell
python -m platformio device monitor -b 115200
```

Commands:

| Command | Meaning |
| --- | --- |
| `?` | Firmware info |
| `H` | Print help |
| `I` | Read current badge ID |
| `Ixxxx` | Set badge ID, four hex digits |
| `T` | Read current team |
| `Tn` | Set team: `0` none, `1` red, `2` green, `3` blue |
| `C` | Read seen badge count |
| `D` | Dump raw seen bitmap |
| `E` | List seen IDs |
| `F!` | Format/clear seen storage |
| `P` | Start pairing |
| `V` | Show own/team/seen IDs |
| `S` | Return to idle; release build then sleeps |
| `Z` or `Z!` | Deep sleep immediately |

## Controls

| Input | Action |
| --- | --- |
| BOOT button on ready screen | Start pairing |
| Touch pad on ready screen | Show own/team/seen IDs |
| BOOT button during show | Switch to pairing |
| RESET button | Wake from final deep sleep |

## Typical Test Flow

1. Build and upload `maker_badge_debug`.
2. Open Serial Monitor at 115200 baud.
3. Send `?` or `H` to confirm the firmware is responding.
4. Send `I` and `T` to check badge identity and team.
5. Use `P` or the BOOT button to start pairing.
6. Use `V` or the touch pad to show stored IDs.
7. Send `Z` to verify the final sleep path.
8. Press RESET to wake the badge again.

## Troubleshooting

| Problem | Check |
| --- | --- |
| IR receiver never sees packets | Receiver OUT must be on GPIO 12 and powered from switched `V+` |
| IR receiver works only while LEDs are on | This is expected because the receiver shares the LED power rail |
| Badge sleeps while debugging | Use `maker_badge_debug` or set `SLEEP_AFTER_ACTION=0` |
| Badge never sleeps on ready screen | Check `IDLE_SLEEP_TIMEOUT_MS`; `0` disables timeout |
| Rev. A display is wrong | Enable `MAKER_BADGE_REV_A_DISPLAY=1` |
| Upload fails | Check USB-C cable, board power switch, and selected serial port |
| Serial Monitor disconnects after action | Release firmware entered deep sleep; press RESET or use debug build |

## Notes

- The IR receiver intentionally shares the LED/NeoPixel power rail. Switching it
  separately would need additional hardware.
- Deep sleep current still depends on real board hardware. Measure current on
  the finished badge because regulators, pull-ups, modules, and wiring can
  dominate the result.
- The Arduino IDE version is kept separately in
  `SW/Arduino/examples/Maker_badge_2019_game`, but this PlatformIO project is
  the main development version.
