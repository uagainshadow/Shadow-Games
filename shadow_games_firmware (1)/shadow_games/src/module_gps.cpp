// ============================================================
// SHADOW GAMES - module_gps.cpp
// ATGM336H GNSS on Cap LoRa868 / Cap LoRa-1262.
// UART pins confirmed from docs.m5stack.com official tutorial:
//   RX = G15, TX = G13, 9600 baud, NMEA sentences.
// ============================================================
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include "event_bus.h"
#include "config.h"

namespace {
    HardwareSerial gpsSerial(1); // UART1
    TinyGPSPlus gps;
}

void gpsModuleTask(void* pv) {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);

    uint32_t lastPublish = 0;
    for (;;) {
        while (gpsSerial.available() > 0) {
            gps.encode(gpsSerial.read());
        }

        if (millis() - lastPublish > 1000) {
            lastPublish = millis();
            ShadowEvent ev{};
            ev.type = EventType::GPS_UPDATE;

            if (gps.location.isValid()) {
                ev.f1 = gps.location.lat();
                ev.f2 = gps.location.lng();
                ev.f3 = gps.altitude.isValid() ? gps.altitude.meters() : 0.0f;
                ev.i1 = gps.satellites.isValid() ? gps.satellites.value() : 0;
                ev.i2 = 1; // fix acquired
                snprintf(ev.text, sizeof(ev.text), "GPS FIX %.5f,%.5f sat:%d",
                          ev.f1, ev.f2, ev.i1);
            } else {
                ev.i2 = 0; // no fix yet
                snprintf(ev.text, sizeof(ev.text), "GPS searching... sat:%d",
                          gps.satellites.isValid() ? gps.satellites.value() : 0);
            }
            EventBus::instance().publish(ev);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
