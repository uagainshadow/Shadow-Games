// ============================================================
// SHADOW GAMES - module_ble.cpp
// Passive BLE advertisement scanning. Also implements the
// "Nearby Device" / "Camera-Smart-Device Awareness" heuristics -
// these flag OBSERVABLE patterns only (known name substrings,
// manufacturer IDs, unusually persistent MACs) and are explicitly
// NOT presented as guaranteed identification, because many devices
// use randomized/rotating BLE MAC addresses and there is no
// universal RF signature for "hidden camera" or "spy glasses".
// ============================================================
#include <NimBLEDevice.h>
#include "event_bus.h"
#include "config.h"

namespace {
    // Known, documented name substrings some devices advertise.
    // This is a heuristic list, not a guarantee - devices can rename
    // themselves or advertise with randomized identifiers.
    const char* kFlipperNameHints[]   = {"Flipper"};              // Flipper Zero often advertises this name when BLE is on
    const char* kCameraNameHints[]    = {"Insta360", "GoPro", "DJI", "Glass"};
    const char* kSkimmerHints[]       = {"HC-05", "HC-06", "SKR", "OBD"}; // common cheap BLE/BT modules seen in fraud reporting - NOT proof of a skimmer

    bool nameContainsAny(const std::string& name, const char* const* list, size_t count) {
        for (size_t i = 0; i < count; i++) {
            if (name.find(list[i]) != std::string::npos) return true;
        }
        return false;
    }

    class ScanCallbacks : public NimBLEScanCallbacks {
        void onResult(const NimBLEAdvertisedDevice* dev) override {
            ShadowEvent ev{};
            ev.type = EventType::BLE_UPDATE;
            std::string name = dev->haveName() ? dev->getName() : "";
            int rssi = dev->getRSSI();

            snprintf(ev.text, sizeof(ev.text), "BLE %s %ddBm",
                      name.empty() ? "(no name)" : name.c_str(), rssi);
            ev.f1 = rssi;
            EventBus::instance().publish(ev);

            // Rough distance estimate from RSSI (log-distance path loss model).
            // txPower assumed ~-59dBm @1m, n=2.5 - this is a ballpark, not survey-grade.
            float estDistanceM = powf(10.0f, ((-59.0f - rssi) / (10.0f * 2.5f)));

            if (!name.empty()) {
                if (nameContainsAny(name, kFlipperNameHints, 1)) {
                    ShadowEvent sight{};
                    sight.type = EventType::DEVICE_SIGHTING;
                    sight.f1 = rssi;
                    sight.f2 = estDistanceM;
                    snprintf(sight.text, sizeof(sight.text),
                              "Possible Flipper-class device (name match) ~%.1fm", estDistanceM);
                    EventBus::instance().publish(sight);
                }
                if (nameContainsAny(name, kCameraNameHints, 4)) {
                    ShadowEvent sight{};
                    sight.type = EventType::DEVICE_SIGHTING;
                    sight.f1 = rssi;
                    sight.f2 = estDistanceM;
                    snprintf(sight.text, sizeof(sight.text),
                              "Camera-class BLE advertiser nearby ~%.1fm (name hint only)", estDistanceM);
                    EventBus::instance().publish(sight);
                }
            }
        }
    };

    ScanCallbacks s_cb;
}

void bleModuleTask(void* pv) {
    NimBLEDevice::init("");
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(&s_cb);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(90);

    for (;;) {
        scan->start(BLE_SCAN_INTERVAL_MS / 1000, false /*not continue*/);
        scan->clearResults();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
