// ============================================================
// SHADOW GAMES - main.cpp
// Boot sequence -> spawns one FreeRTOS task per subsystem ->
// UI task drains the shared event bus at ~UI_REFRESH_HZ and
// redraws the dashboard. Modules never touch the display directly,
// so a busy WiFi/BLE scan can't stall the UI.
// ============================================================
#include <M5Unified.h>
#include "config.h"
#include "event_bus.h"
#include "ui_theme.h"
#include "tasks.h"

namespace {
    M5Canvas canvas(&M5.Display);

    struct DashboardState {
        int wifiAps = 0;
        int bleDevices = 0;
        uint32_t deauthCount = 0;
        char deauthText[96] = "no threats";
        bool deauthAlert = false;
        char deviceSighting[96] = "-";
        bool rfidHwPresent = false;
        char rfidText[64] = "no hardware";
        bool gpsFix = false;
        float gpsLat = 0, gpsLon = 0;
        int gpsSats = 0;
        char loraText[96] = "init...";
        float loraRssi = 0;
        int loraPackets = 0;
        char logFeed[6][80];
        int logHead = 0;
        float radarAngle = 0;
    } state;

    void pushLog(const char* line) {
        state.logHead = (state.logHead + 1) % 6;
        strncpy(state.logFeed[state.logHead], line, sizeof(state.logFeed[0]) - 1);
    }

    void handleEvent(const ShadowEvent& ev) {
        switch (ev.type) {
            case EventType::WIFI_UPDATE:
                state.wifiAps = ev.i1;
                pushLog(ev.text);
                break;
            case EventType::BLE_UPDATE:
                state.bleDevices++;
                break;
            case EventType::DEAUTH_ALERT:
                state.deauthCount++;
                state.deauthAlert = true;
                strncpy(state.deauthText, ev.text, sizeof(state.deauthText) - 1);
                pushLog(ev.text);
                break;
            case EventType::DEVICE_SIGHTING:
                strncpy(state.deviceSighting, ev.text, sizeof(state.deviceSighting) - 1);
                pushLog(ev.text);
                break;
            case EventType::RFID_UPDATE:
                state.rfidHwPresent = (ev.i1 >= 0);
                strncpy(state.rfidText, ev.text, sizeof(state.rfidText) - 1);
                break;
            case EventType::GPS_UPDATE:
                state.gpsFix = (ev.i2 == 1);
                if (state.gpsFix) {
                    state.gpsLat = ev.f1;
                    state.gpsLon = ev.f2;
                    state.gpsSats = ev.i1;
                }
                break;
            case EventType::LORA_UPDATE:
                strncpy(state.loraText, ev.text, sizeof(state.loraText) - 1);
                state.loraRssi = ev.f1;
                state.loraPackets = ev.i1;
                break;
            case EventType::LOG_LINE:
                pushLog(ev.text);
                break;
        }
    }

    void bootSequence() {
        const char* steps[] = {
            "Initializing CPU",
            "Loading RF Drivers",
            "Loading WiFi",
            "Loading Bluetooth",
            "Loading GPS",
            "Loading LoRa",
            "Loading Dashboard",
            "Ready"
        };
        M5.Display.fillScreen(theme::BG);
        M5.Display.setTextColor(theme::PHOS_HOT, theme::BG);
        M5.Display.setTextSize(2);
        M5.Display.setCursor(10, 10);
        M5.Display.print("SHADOW GAMES");
        M5.Display.setTextSize(1);
        int y = 40;
        for (auto s : steps) {
            M5.Display.setCursor(10, y);
            M5.Display.setTextColor(theme::PHOS_MID, theme::BG);
            M5.Display.print(s);
            delay(120);
            M5.Display.setTextColor(theme::PHOS_HOT, theme::BG);
            M5.Display.print("  [OK]");
            y += 12;
            delay(60);
        }
        delay(300);
    }

    void drawDashboard() {
        canvas.fillSprite(theme::BG);
        int w = M5.Display.width();
        int h = M5.Display.height();

        canvas.setTextColor(theme::PHOS_HOT, theme::BG);
        canvas.setCursor(4, 2);
        canvas.print("SHADOW GAMES");

        // --- Radar (center) ---
        int cx = w / 2, cy = h / 2 + 10, radius = min(w, h) / 4;
        theme::radarSweep(canvas, cx, cy, radius, state.radarAngle);
        state.radarAngle += 6.0f;
        if (state.radarAngle >= 360.0f) state.radarAngle = 0;

        // --- Left column: WiFi / BLE / Deauth ---
        int lx = 2, ly = 16, lw = w / 2 - 4;
        theme::panelFrame(canvas, lx, ly, lw, 22, "WIFI");
        canvas.setCursor(lx + 2, ly + 12);
        canvas.printf("APs: %d", state.wifiAps);

        theme::panelFrame(canvas, lx, ly + 24, lw, 22, "BLUETOOTH");
        canvas.setCursor(lx + 2, ly + 36);
        canvas.printf("Devices: %d", state.bleDevices);

        theme::panelFrame(canvas, lx, ly + 48, lw, 22,
                           state.deauthAlert ? "DEAUTH !!" : "DEAUTH");
        canvas.setTextColor(state.deauthAlert ? theme::RED_ALERT : theme::PHOS_MID, theme::BG);
        canvas.setCursor(lx + 2, ly + 60);
        canvas.print(state.deauthText);
        canvas.setTextColor(theme::PHOS_MID, theme::BG);

        // --- Right column: GPS / LoRa / Devices ---
        int rx = w / 2 + 2, ry = 16, rw = w / 2 - 4;
        theme::panelFrame(canvas, rx, ry, rw, 22, "GPS");
        canvas.setCursor(rx + 2, ry + 12);
        if (state.gpsFix) {
            canvas.printf("%.4f,%.4f s:%d", state.gpsLat, state.gpsLon, state.gpsSats);
        } else {
            canvas.print("searching...");
        }

        theme::panelFrame(canvas, rx, ry + 24, rw, 22, "LORA");
        canvas.setCursor(rx + 2, ry + 36);
        canvas.printf("RSSI:%.0f pkts:%d", state.loraRssi, state.loraPackets);

        theme::panelFrame(canvas, rx, ry + 48, rw, 22, "DEVICE SIGHTING");
        canvas.setCursor(rx + 2, ry + 60);
        canvas.print(state.deviceSighting);

        // --- Bottom: RFID + notification feed ---
        int by = h - 60;
        theme::panelFrame(canvas, 2, by, w - 4, 10, "RFID");
        canvas.setCursor(4, by + 10);
        canvas.print(state.rfidHwPresent ? state.rfidText : "no reader hardware attached");

        theme::panelFrame(canvas, 2, by + 20, w - 4, 10, "EVENT FEED");
        for (int i = 0; i < 4; i++) {
            int idx = (state.logHead - i + 6) % 6;
            canvas.setCursor(4, by + 20 + 9 + i * 9);
            canvas.print(state.logFeed[idx]);
        }

        canvas.pushSprite(0, 0);
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    canvas.createSprite(M5.Display.width(), M5.Display.height());
    theme::applyDefaults(canvas);

    Serial.begin(115200);
    EventBus::instance().begin();

    bootSequence();

    // One task per subsystem, per the shared-event-bus architecture.
    // Stack sizes are generous starting points - trim after profiling.
    xTaskCreatePinnedToCore(wifiModuleTask, "wifi", 6144, nullptr, 3, nullptr, 0);
    xTaskCreatePinnedToCore(bleModuleTask,  "ble",  6144, nullptr, 3, nullptr, 1);
    xTaskCreatePinnedToCore(gpsModuleTask,  "gps",  4096, nullptr, 2, nullptr, 1);
    xTaskCreatePinnedToCore(loraModuleTask, "lora", 4096, nullptr, 2, nullptr, 1);
    xTaskCreatePinnedToCore(rfidModuleTask, "rfid", 2048, nullptr, 1, nullptr, 1);
}

void loop() {
    M5.update();
    EventBus::instance().drain(handleEvent);
    drawDashboard();
    delay(1000 / UI_REFRESH_HZ);
}
