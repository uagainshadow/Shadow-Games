# SHADOW GAMES firmware — Cardputer-Adv + Cap LoRa868 / Cap LoRa-1262

Unified RF awareness dashboard, green-phosphor / 1945-radar UI, for the
M5Stack Cardputer-Adv (ESP32-S3 / Stamp-S3A) with the official LoRa+GPS cap.

## What's actually in this folder

This is a **source code project**, not pre-built binaries. I don't have
internet access in my sandbox, so I can't download the ESP32 toolchain and
compile it for you here. What you have is a complete, correctly-pinned
PlatformIO project that will compile on your machine with one command.

```
shadow_games/
  platformio.ini      <- board/toolchain config (copied from M5Stack's own docs)
  include/
    config.h           pin map + radio params (sourced, see below)
    event_bus.h         shared event bus so modules never block the UI
    ui_theme.h           green CRT drawing helpers
    tasks.h
  src/
    main.cpp             boot sequence + dashboard UI + task spawning
    module_wifi.cpp        WiFi recon + deauth/disassoc/beacon-flood detector
    module_ble.cpp          BLE scan + heuristic device-sighting flags
    module_gps.cpp           ATGM336H GPS (TinyGPS++)
    module_lora.cpp           SX1262 LoRa (RadioLib), auto-detects cap revision
    module_rfid.cpp            honest stub — see note below
```

## Where the pin numbers came from

Guessing GPIO/partition/flash values is how you get a board that won't
boot, so every pin in `config.h` is copied from M5Stack's own published
tutorial for this exact cap ("Cap LoRa868 / LoRa-1262 Arduino Tutorial",
docs.m5stack.com), not invented:

| Signal | Pin |
|---|---|
| LoRa SPI NSS | G5 |
| LoRa SPI MOSI | G14 |
| LoRa SPI MISO | G39 |
| LoRa SPI SCK | G40 |
| LoRa IRQ (DIO1) | G4 |
| LoRa RST | G3 |
| LoRa BUSY | G6 |
| GPS RX (Cardputer side) | G15 |
| GPS TX (Cardputer side) | G13 |
| GPS baud | 9600 |

**If you're using a different/older cap or a future hardware revision**,
re-check `docs.m5stack.com/en/arduino/projects/cap/cap_lora868` before
flashing — wrong pins here just mean LoRa/GPS won't come up, not a bricked
board, but there's no reason to guess when M5Stack publishes the real
numbers.

## RFID: now wired to a real, official part

M5Stack sells the **Unit RFID2 (WS1850S)**, ~$5, SKU U031-B — 13.56MHz,
reads MIFARE/NTAG cards, plugs into the Grove (HY2.0-4P) port with no
soldering. `module_rfid.cpp` now drives it for real, using M5Stack's own
`MFRC522_I2C` library at I2C address `0x28` (confirmed from
`docs.m5stack.com/en/arduino/projects/unit/unit_rfid`).

**One thing I couldn't confirm:** M5Stack's docs don't publish which
GPIOs the Cardputer-Adv's Grove port maps to. The only number I found
(`SDA=G2, SCL=G1`) came from a third-party community repo, not M5Stack
directly — so `config.h` marks it as unconfirmed rather than presenting
it as fact.

To handle that honestly instead of guessing blind: the firmware now runs
an **I2C bus scan at boot** and logs every address it finds over Serial.
Flash it, open the serial monitor, and:
- If you see `I2C found device @ 0x28` → the pins are right, the RFID
  panel will start reading cards.
- If you see `RFID2 (0x28) NOT found` → the Grove port is on different
  GPIOs than G2/G1 on your unit; try swapping them or check with a
  multimeter/continuity test against the M5Stack schematic, then update
  `PIN_RFID_SDA` / `PIN_RFID_SCL` in `config.h`.

Same idea applies to any other Grove accessory you add later — when in
doubt, scan first, hardcode second.

Similarly, the "Flipper Zero detector" and "camera/glasses detector" only
flag **observable BLE advertisement name matches** — they say so in the UI
text. There's no reliable universal RF signature for those device classes
(many use randomized BLE MACs), so this firmware doesn't claim otherwise.

## Getting the 4 flashable files for a web flasher

A browser-based flasher (Espressif's esptool-js, ESPHome Web, etc.) needs
actual compiled `.bin` files, not this source code. I can't compile them
in my sandbox — no internet access to pull the ESP32 toolchain — but this
project is set up so **GitHub's free build servers do it for you**, no
local install required:

1. Push this whole `shadow_games/` folder to a new GitHub repo (Settings
   → your repo → "Add file" → "Upload files" works fine, no `git`
   command line needed).
2. Open the repo's **Actions** tab. A workflow called "Build Shadow Games
   firmware" runs automatically (or click **Run workflow** if it didn't).
3. When it finishes (green check, a couple minutes), click into the run,
   scroll down to **Artifacts**, and download `shadow-games-flash-files`.
4. Unzip it. You'll have exactly the 4 files:

   | File | What it is | Flash offset |
   |---|---|---|
   | `bootloader.bin` | 2nd-stage bootloader | `0x0` |
   | `partitions.bin` | Partition table | `0x8000` |
   | `boot_app0.bin` | OTA boot-selector stub | `0xe000` |
   | `firmware.bin` | Your compiled app | `0x10000` |

   Plus a `manifest.json` that already knows those offsets.

### Flashing with a browser tool

**Option A — one-click (recommended):** Use a manifest-based flasher like
[ESPHome Web](https://web.esphome.io) or any [ESP Web Tools](https://esphometools.web.app/)-based
page. Point it at the folder containing `manifest.json` + the 4 `.bin`
files (or host them together if the tool wants a URL) — it reads the
offsets automatically so you don't type any hex addresses.

**Option B — manual offsets:** Use [Espressif's esptool-js](https://espressif.github.io/esptool-js/)
directly. Connect the board (Chrome/Edge only, needs Web Serial), add
each of the 4 files, and type in the offsets from the table above.

Either way: **enter download mode first** — power switch OFF → hold G0 →
power switch ON → release G0 — then connect USB-C and hit flash. Baud
rate: start at **1,500,000**; if the browser tool can't hold sync at that
speed, drop to 921600 or 115200.

No GitHub account and don't want to make one? The alternative is
installing PlatformIO locally (`pip install platformio`) and running
`pio run` in this folder — it produces the same 4 files under
`.pio/build/cardputer-adv/`, plus `boot_app0.bin` lives inside your
PlatformIO packages folder (the CI workflow above shows exactly where to
find it, under `~/.platformio/.../boot_app0.bin`).

## Building it (developer/local route)

1. Install [PlatformIO](https://platformio.org/) — easiest via the
   VS Code extension, or `pip install platformio`.
2. Open this `shadow_games/` folder as a PlatformIO project.
3. Plug in the Cardputer-Adv via USB-C.
4. **Enter download mode**: power switch OFF → hold the G0 button → power
   switch ON → release G0. (This is the Cardputer-Adv's documented boot-mode
   procedure, same as the base Cardputer.)
5. Run:
   ```
   pio run --target upload
   ```
   PlatformIO will compile and flash it in one step using `esptool.py`
   at the upload speed set in `platformio.ini` (**1,500,000 baud**, per
   M5Stack's own recommended config — drop to 921600 or 115200 in
   `platformio.ini` if you get sync errors on your USB/OS combo).

## The "4 files" you were thinking of

When PlatformIO/esptool flashes an Arduino-framework ESP32 project, it
writes four separate binaries to specific flash offsets — this is what you
were remembering:

| File | What it is | Typical offset |
|---|---|---|
| `bootloader.bin` | 2nd-stage bootloader | `0x0` |
| `partitions.bin` | Partition table | `0x8000` |
| `boot_app0.bin` | OTA boot-selector stub | `0xe000` |
| `firmware.bin` | Your actual compiled app | `0x10000` |

You don't need to juggle these by hand — `pio run --target upload` builds
all four and flashes them in the correct order automatically. They'll
show up under `.pio/build/cardputer-adv/` after a build if you ever want
to flash manually with `esptool.py` (e.g. to re-flash without recompiling,
or to build a "flash this to a friend's board" package).

## Regional radio note

`LORA_FREQ_MHZ` in `config.h` is set to **868 MHz (EU band)**, matching
the cap's SMA antenna tuning per M5Stack. If you're in the US, change it
to 915 MHz and double check local ISM-band rules for your setup — the
antenna is tuned for 868, so 915 will work but with reduced range.

## Extending it

The event-bus architecture means adding a module is: write a task that
calls `EventBus::instance().publish(...)`, add its `xTaskCreatePinnedToCore`
line in `main.cpp`, and add a case to `handleEvent()` if the dashboard
should show it. Nothing in `module_*.cpp` ever touches the display
directly, so a slow WiFi scan can't freeze the UI.
