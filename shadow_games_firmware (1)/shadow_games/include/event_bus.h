#pragma once
// ============================================================
// SHADOW GAMES - event_bus.h
// A tiny FreeRTOS-queue-based pub/sub bus. Each subsystem task
// pushes status snapshots here; the UI task drains it every frame
// and never blocks waiting on a sensor/radio.
// ============================================================
#include <Arduino.h>

enum class EventType : uint8_t {
    WIFI_UPDATE,
    BLE_UPDATE,
    DEAUTH_ALERT,
    DEVICE_SIGHTING,     // generic "nearby device" (Flipper-heuristic, cam-heuristic)
    RFID_UPDATE,
    GPS_UPDATE,
    LORA_UPDATE,
    LOG_LINE,            // free text -> notification feed
};

struct ShadowEvent {
    EventType type;
    char      text[96];   // human-readable summary, always safe to print
    float     f1 = 0, f2 = 0, f3 = 0;   // generic numeric payload slots
    int32_t   i1 = 0, i2 = 0;
    uint32_t  timestamp_ms = 0;
};

class EventBus {
public:
    static EventBus& instance() {
        static EventBus bus;
        return bus;
    }

    void begin(size_t queueLen = 64) {
        _queue = xQueueCreate(queueLen, sizeof(ShadowEvent));
    }

    // Called from any task (WiFi/BLE/GPS/LoRa/etc). Non-blocking.
    bool publish(const ShadowEvent& ev) {
        if (!_queue) return false;
        ShadowEvent copy = ev;
        copy.timestamp_ms = millis();
        return xQueueSend(_queue, &copy, 0) == pdTRUE;
    }

    // Called only from the UI task. Drains everything currently queued.
    // Returns number of events processed.
    template <typename Fn>
    size_t drain(Fn&& handler, size_t maxEvents = 32) {
        if (!_queue) return 0;
        ShadowEvent ev;
        size_t n = 0;
        while (n < maxEvents && xQueueReceive(_queue, &ev, 0) == pdTRUE) {
            handler(ev);
            n++;
        }
        return n;
    }

private:
    EventBus() = default;
    QueueHandle_t _queue = nullptr;
};
