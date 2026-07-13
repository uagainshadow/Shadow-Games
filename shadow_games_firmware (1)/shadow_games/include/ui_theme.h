#pragma once
// ============================================================
// SHADOW GAMES - ui_theme.h
// 1945-radar / green-phosphor CRT look, no cyberpunk neon.
// ============================================================
#include <M5Unified.h>

namespace theme {
    // Green phosphor palette
    constexpr uint16_t BG        = 0x0000; // near-black
    constexpr uint16_t PHOS_DIM  = 0x0320; // dim green (grid lines)
    constexpr uint16_t PHOS_MID  = 0x0540; // mid green (text)
    constexpr uint16_t PHOS_HOT  = 0x07E0; // bright green (highlights)
    constexpr uint16_t AMBER     = 0xFD20; // amber warning
    constexpr uint16_t RED_ALERT = 0xF800; // deauth / threat red

    inline void applyDefaults(M5Canvas& c) {
        c.setTextColor(PHOS_MID, BG);
        c.setFont(&fonts::Font0); // swap for a monospaced/typewriter font if you add one to /data
    }

    inline void panelFrame(M5Canvas& c, int x, int y, int w, int h, const char* label) {
        c.drawRect(x, y, w, h, PHOS_DIM);
        c.fillRect(x, y, w, 10, PHOS_DIM);
        c.setTextColor(BG, PHOS_DIM);
        c.setCursor(x + 3, y + 1);
        c.print(label);
        c.setTextColor(PHOS_MID, BG);
    }

    // Simple rotating radar sweep line, angleDeg advances each frame.
    inline void radarSweep(M5Canvas& c, int cx, int cy, int radius, float angleDeg) {
        for (int r = radius; r > 0; r -= radius / 4) {
            c.drawCircle(cx, cy, r, PHOS_DIM);
        }
        float rad = angleDeg * DEG_TO_RAD;
        int ex = cx + (int)(radius * cosf(rad));
        int ey = cy + (int)(radius * sinf(rad));
        c.drawLine(cx, cy, ex, ey, PHOS_HOT);
    }

    inline void blip(M5Canvas& c, int x, int y, uint16_t color = PHOS_HOT) {
        c.fillCircle(x, y, 2, color);
    }
}
