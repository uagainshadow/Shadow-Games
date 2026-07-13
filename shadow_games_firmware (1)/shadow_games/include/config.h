#pragma once
// ============================================================
// SHADOW GAMES - config.h
// Hardware pin map for Cardputer-Adv + Cap LoRa868 / Cap LoRa-1262
//
// SOURCE OF TRUTH: docs.m5stack.com/en/arduino/projects/cap/cap_lora868
// (M5Stack's own official tutorial page for this exact cap). Values
// below are copied verbatim from that page, not guessed.
//
// If you are using a DIFFERENT cap revision, or M5Stack changes the
// pinout in a future hardware rev, confirm against the current page
// before flashing - wrong SPI/UART pins won't brick the board, but
// LoRa/GPS just won't come up.
// ============================================================

// ---- Cap LoRa868 / Cap LoRa-1262 : LoRa (SX1262) via SPI ----
#define PIN_LORA_NSS    5   // G5  - SPI chip select
#define PIN_LORA_MOSI   14  // G14 - SPI MOSI (auto-mapped by M5Unified too)
#define PIN_LORA_MISO   39  // G39 - SPI MISO
#define PIN_LORA_SCK    40  // G40 - SPI SCK
#define PIN_LORA_IRQ    4   // G4  - DIO1 / IRQ
#define PIN_LORA_RST    3   // G3  - Reset
#define PIN_LORA_BUSY   6   // G6  - Busy line

// ---- Cap LoRa868 / Cap LoRa-1262 : GPS (ATGM336H) via UART ----
#define PIN_GPS_RX      15  // G15 - Cardputer RX  <- GPS TX
#define PIN_GPS_TX      13  // G13 - Cardputer TX  -> GPS RX
#define GPS_BAUD        9600

// ---- Cap LoRa-1262 revision detection (I2C IO expander) ----
// The newer Cap LoRa-1262 adds a PI4IOE5V6408 IO expander on the
// shared I2C bus (addr 0x43) used to enable an antenna switch.
// If it responds, you have the -1262 cap; if not, you have LoRa868.
// M5Unified's default I2C bus handles this automatically -
// see src/module_lora.cpp for the detection code.
#define LORA_CAP_IOEXP_ADDR 0x43

// ---- LoRa radio parameters (regional!) ----
// 868 MHz = EU ISM band. If you are in the US, this MUST be changed
// to 915 MHz (and confirm local regulations for duty cycle / power).
#define LORA_FREQ_MHZ     868.0f
#define LORA_BW_KHZ       125.0f
#define LORA_SF            9      // lower than 12 -> faster UI refresh, still long range
#define LORA_CR             7
#define LORA_SYNC_WORD    0x34
#define LORA_TX_POWER_DBM  17     // keep conservative; region limits may be lower
#define LORA_PREAMBLE_LEN  8

// ---- Unit RFID2 (WS1850S) via Grove I2C port ----
// Official M5Stack part, confirmed compatible: I2C addr 0x28, uses the
// MFRC522_I2C library (docs.m5stack.com/en/arduino/projects/unit/unit_rfid).
//
// GPIO NOTE: M5Stack's own docs don't publish which GPIOs the
// Cardputer-Adv's Grove port maps to. The only number I could find
// (SDA=G2, SCL=G1) comes from a third-party community test repo, not
// M5Stack directly - so treat it as a starting guess, not a fact.
// This firmware runs an I2C bus scan at boot (see module_rfid.cpp) and
// prints every address it finds to Serial, so you can confirm 0x28
// shows up on first flash before trusting these pins.
#define PIN_RFID_SDA   2   // G2 - UNCONFIRMED, verify via boot-time I2C scan
#define PIN_RFID_SCL   1   // G1 - UNCONFIRMED, verify via boot-time I2C scan
#define RFID_I2C_ADDR  0x28

// ---- General expansion bus reference (from M5Stack manual) ----
// 14-pin header: G3 G4 G5 G6 G8 G9 G13 G14 G15 G39 G40 SDA SCL 5V GND
// Grove HY2.0-4P: 5V, GND, G13(SDA)/G15(SCL) depending on mode - Grove
// port is shared logically with some of the above; the caps use the
// expansion bus, not Grove, so no conflict for this build.

// ---- UI refresh / task timing ----
#define UI_REFRESH_HZ        10
#define WIFI_SCAN_INTERVAL_MS  4000
#define BLE_SCAN_INTERVAL_MS   4000
#define DEAUTH_WINDOW_MS       1000

// ---- Boot mode reminder (printed in README too) ----
// To flash: power switch OFF -> hold G0 -> power ON -> release G0.
