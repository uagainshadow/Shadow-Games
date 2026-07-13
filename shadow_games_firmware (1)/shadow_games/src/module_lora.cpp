// ============================================================
// SHADOW GAMES - module_lora.cpp
// SX1262 via RadioLib. SPI/control pins confirmed from M5Stack's
// official Cap LoRa868/LoRa-1262 Arduino tutorial page.
// Auto-detects Cap LoRa-1262 (has PI4IOE5V6408 IO expander on I2C)
// vs older Cap LoRa868 (does not).
// ============================================================
#include <M5Unified.h>
#include <RadioLib.h>
#include "event_bus.h"
#include "config.h"

namespace {
    SX1262 radio = new Module(PIN_LORA_NSS, PIN_LORA_IRQ, PIN_LORA_RST, PIN_LORA_BUSY);
    volatile bool s_gotPacket = false;

    void IRAM_ATTR onLoraIrq() { s_gotPacket = true; }

    bool detectCapIs1262() {
        M5.In_I2C.begin();
        M5.In_I2C.start(LORA_CAP_IOEXP_ADDR, false, 400000);
        bool ok = M5.In_I2C.stop();
        return ok; // if the IO expander ACKs, this is the newer Cap LoRa-1262
    }
}

void loraModuleTask(void* pv) {
    bool is1262 = detectCapIs1262();

    ShadowEvent detEv{};
    detEv.type = EventType::LOG_LINE;
    snprintf(detEv.text, sizeof(detEv.text), "Cap detected: %s",
              is1262 ? "LoRa-1262" : "LoRa868");
    EventBus::instance().publish(detEv);

    int state = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                              LORA_SYNC_WORD, LORA_TX_POWER_DBM, LORA_PREAMBLE_LEN, 3.0, true);

    ShadowEvent initEv{};
    initEv.type = EventType::LOG_LINE;
    if (state == RADIOLIB_ERR_NONE) {
        snprintf(initEv.text, sizeof(initEv.text), "LoRa init OK @ %.1fMHz", LORA_FREQ_MHZ);
    } else {
        snprintf(initEv.text, sizeof(initEv.text), "LoRa init FAILED code %d", state);
    }
    EventBus::instance().publish(initEv);

    radio.setDio1Action(onLoraIrq);
    radio.startReceive();

    uint32_t packetCount = 0;
    for (;;) {
        if (s_gotPacket) {
            s_gotPacket = false;
            String payload;
            int rState = radio.readData(payload);
            if (rState == RADIOLIB_ERR_NONE) {
                packetCount++;
                ShadowEvent ev{};
                ev.type = EventType::LORA_UPDATE;
                ev.f1 = radio.getRSSI();
                ev.f2 = radio.getSNR();
                ev.i1 = packetCount;
                snprintf(ev.text, sizeof(ev.text), "LoRa RX #%lu RSSI:%.0f SNR:%.1f",
                          (unsigned long)packetCount, ev.f1, ev.f2);
                EventBus::instance().publish(ev);
            }
            radio.startReceive();
        }

        // Periodic status heartbeat even with no packets, so the UI
        // panel shows "radio alive" rather than looking frozen.
        static uint32_t lastHeartbeat = 0;
        if (millis() - lastHeartbeat > 2000) {
            lastHeartbeat = millis();
            ShadowEvent ev{};
            ev.type = EventType::LORA_UPDATE;
            ev.f1 = radio.getRSSI();
            ev.i1 = packetCount;
            snprintf(ev.text, sizeof(ev.text), "LoRa idle, %lu pkts total", (unsigned long)packetCount);
            EventBus::instance().publish(ev);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
