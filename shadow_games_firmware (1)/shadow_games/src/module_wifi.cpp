// ============================================================
// SHADOW GAMES - module_wifi.cpp
// Passive WiFi recon + management-frame based deauth/disassoc/
// beacon-flood DETECTION. This module never transmits deauth
// frames - it only listens in promiscuous mode and counts them.
// ============================================================
#include <WiFi.h>
#include <esp_wifi.h>
#include "event_bus.h"
#include "config.h"

namespace {
    struct ApRecord {
        String ssid;
        int32_t rssi;
        uint8_t channel;
        wifi_auth_mode_t auth;
    };

    volatile uint32_t s_deauthCount = 0;
    volatile uint32_t s_disassocCount = 0;
    volatile uint32_t s_beaconCount = 0;
    uint32_t s_windowStart = 0;

    // Promiscuous callback - runs in WiFi driver context, keep it FAST.
    // We only tally counters here; the heavier reporting happens in the task.
    void IRAM_ATTR promiscCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
        if (type != WIFI_PKT_MGMT) return;
        const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
        const uint8_t* payload = pkt->payload;
        uint8_t frameSubType = (payload[0] & 0xF0) >> 4;
        // 802.11 mgmt subtypes: 0x8=beacon, 0xA=disassoc, 0xC=deauth
        switch (frameSubType) {
            case 0xC: s_deauthCount++; break;
            case 0xA: s_disassocCount++; break;
            case 0x8: s_beaconCount++; break;
            default: break;
        }
    }
}

void wifiModuleTask(void* pv) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&promiscCallback);

    uint8_t channel = 1;
    s_windowStart = millis();

    for (;;) {
        // --- Active scan pass (blocks briefly, promiscuous cb still fires) ---
        esp_wifi_set_promiscuous(false);
        int n = WiFi.scanNetworks(false, true /*show hidden*/, false, 200 /*ms per channel*/);

        ShadowEvent ev{};
        ev.type = EventType::WIFI_UPDATE;
        ev.i1 = n;
        snprintf(ev.text, sizeof(ev.text), "WiFi scan: %d networks", n);
        EventBus::instance().publish(ev);

        for (int i = 0; i < n; i++) {
            ShadowEvent apEv{};
            apEv.type = EventType::LOG_LINE;
            snprintf(apEv.text, sizeof(apEv.text), "AP %s ch%d %ddBm",
                      WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
            apEv.f1 = WiFi.RSSI(i);
            apEv.i1 = WiFi.channel(i);
            EventBus::instance().publish(apEv);
        }
        WiFi.scanDelete();

        // --- Deauth/disassoc/beacon-flood monitoring window ---
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        channel = (channel % 11) + 1; // hop 1..11 (2.4GHz, US/EU-safe subset)

        s_deauthCount = 0;
        s_disassocCount = 0;
        s_beaconCount = 0;
        uint32_t windowStart = millis();
        while (millis() - windowStart < DEAUTH_WINDOW_MS) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        uint32_t deauths = s_deauthCount;
        uint32_t disassocs = s_disassocCount;
        uint32_t beacons = s_beaconCount;

        // Simple thresholds - tune per environment. A handful of deauths
        // happens on any real network; sustained bursts are the signal.
        if (deauths > 10 || disassocs > 10) {
            ShadowEvent alert{};
            alert.type = EventType::DEAUTH_ALERT;
            alert.i1 = deauths;
            alert.i2 = disassocs;
            alert.f1 = channel;
            snprintf(alert.text, sizeof(alert.text),
                      "DEAUTH ch%d: %lu deauth / %lu disassoc /1s",
                      channel, (unsigned long)deauths, (unsigned long)disassocs);
            EventBus::instance().publish(alert);
        }
        if (beacons > 300) {
            ShadowEvent alert{};
            alert.type = EventType::DEAUTH_ALERT;
            alert.i1 = beacons;
            snprintf(alert.text, sizeof(alert.text),
                      "BEACON FLOOD ch%d: %lu beacons/1s", channel, (unsigned long)beacons);
            EventBus::instance().publish(alert);
        }
    }
}
