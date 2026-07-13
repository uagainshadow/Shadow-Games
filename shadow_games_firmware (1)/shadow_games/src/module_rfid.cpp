// ============================================================
// SHADOW GAMES - module_rfid.cpp
// M5Stack "Unit RFID2" (WS1850S), 13.56MHz, official Grove-port
// accessory. Uses M5Stack's own MFRC522_I2C library at addr 0x28.
//
// The SDA/SCL GPIOs in config.h are UNCONFIRMED (see the comment
// there) - so before trusting them, this task runs a plain I2C bus
// scan at boot and logs every address it finds. If 0x28 doesn't show
// up, the pins are wrong for your unit and need adjusting in
// config.h - the firmware will keep reporting "no reader detected"
// rather than pretending to read cards.
// ============================================================
#include <Wire.h>
#include <MFRC522_I2C.h>
#include "event_bus.h"
#include "config.h"

namespace {
    MFRC522_I2C mfrc522(RFID_I2C_ADDR, -1);
    bool s_readerPresent = false;

    void i2cScanAndLog() {
        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                ShadowEvent ev{};
                ev.type = EventType::LOG_LINE;
                snprintf(ev.text, sizeof(ev.text), "I2C found device @ 0x%02X", addr);
                EventBus::instance().publish(ev);
                if (addr == RFID_I2C_ADDR) s_readerPresent = true;
            }
        }
        if (!s_readerPresent) {
            ShadowEvent ev{};
            ev.type = EventType::LOG_LINE;
            snprintf(ev.text, sizeof(ev.text), "RFID2 (0x28) NOT found - check SDA/SCL pins");
            EventBus::instance().publish(ev);
        }
    }
}

void rfidModuleTask(void* pv) {
    Wire.begin(PIN_RFID_SDA, PIN_RFID_SCL);
    i2cScanAndLog();

    if (s_readerPresent) {
        mfrc522.PCD_Init();
    }

    for (;;) {
        ShadowEvent ev{};
        ev.type = EventType::RFID_UPDATE;

        if (!s_readerPresent) {
            ev.i1 = -1; // sentinel: no hardware found
            snprintf(ev.text, sizeof(ev.text), "RFID: no reader detected on I2C bus");
        } else if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            char uidStr[32] = {0};
            int pos = 0;
            for (byte i = 0; i < mfrc522.uid.size && pos < (int)sizeof(uidStr) - 3; i++) {
                pos += snprintf(uidStr + pos, sizeof(uidStr) - pos, "%02X", mfrc522.uid.uidByte[i]);
            }
            ev.i1 = 1; // card present
            snprintf(ev.text, sizeof(ev.text), "RFID card UID: %s", uidStr);
            EventBus::instance().publish(ev); // also drop it in the log feed
            mfrc522.PICC_HaltA();
        } else {
            ev.i1 = 0; // reader present, no card
            snprintf(ev.text, sizeof(ev.text), "RFID: reader idle");
        }

        EventBus::instance().publish(ev);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
