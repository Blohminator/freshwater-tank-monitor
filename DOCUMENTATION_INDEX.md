# Documentation Index — Freshwater Tank Monitor

A complete overview of all project documentation.

---

## Start here

| Document | Description |
|----------|-------------|
| [QUICKSTART.md](QUICKSTART.md) | Get up and running in 5 steps — wiring, flashing, WiFi setup, tank config |
| [README.md](README.md) | Full project documentation — features, setup, SignalK paths, specifications |

---

## Hardware

| Document | Description |
|----------|-------------|
| [WIRING.md](WIRING.md) | Detailed wiring diagrams, pin assignments, power requirements, assembly steps |

---

## Configuration

All runtime configuration is done via the web interface at `http://freshwater-tank.local`.

> � No login required — the web interface opens directly in your browser.

| Path | Parameter | Default |
|------|-----------|---------|
| `/Tank/Length_cm` | Tank length in cm | 100 |
| `/Tank/Width_cm` | Tank width in cm | 50 |
| `/Tank/Height_cm` | Tank height in cm | 110 |
| `/Tank/Offset_cm` | Sensor offset in cm (sensor face to tank top edge) | 5 |
| `/Tank/Alarm_pct` | Alarm threshold in % | 95 |

No recompilation needed — changes take effect immediately.

---

## SignalK Data Paths

| Path | Type | Unit | Update |
|------|------|------|--------|
| `tanks.freshWater.0.currentLevel` | float | ratio 0.0–1.0 | every valid measurement (~100 ms) |
| `tanks.freshWater.0.capacity` | float | m³ | on startup + every 60s |
| `tanks.freshWater.0.currentVolume` | float | m³ | every valid measurement (~100 ms) |

---

## Troubleshooting & Development

| Document | Description |
|----------|-------------|
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Common problems and fixes — display, LiDAR, WiFi, SignalK, alarm |
| [NEXT_STEPS.md](NEXT_STEPS.md) | Planned improvements, feature ideas, known limitations |
| [CHANGELOG.md](CHANGELOG.md) | Version history and change log |

---

## Localization

| Document | Description |
|----------|-------------|
| [README_DE.md](README_DE.md) | Full documentation in German |

---

## Project Structure

```
freshwater-tank-monitor/
├── src/
│   └── main.cpp              # Main firmware source
├── platformio.ini            # PlatformIO build configuration
├── DOCUMENTATION_INDEX.md    # This file
├── QUICKSTART.md             # 5-step getting started guide
├── README.md                 # Full English documentation
├── README_DE.md              # Full German documentation
├── WIRING.md                 # Wiring diagrams and pin reference
├── TROUBLESHOOTING.md        # Problem diagnosis and fixes
├── NEXT_STEPS.md             # Future improvements and known limits
└── CHANGELOG.md              # Version history
```

---

## Hardware Summary

| Component | Spec |
|-----------|------|
| Microcontroller | ESP32-WROOM-32 |
| Distance sensor | TFmini-S LiDAR (UART, 5V) |
| Display | SH1106 128x64 OLED (I2C, 3.3V) |
| Alarm output | GPIO 23 (3.3V logic) |
| Framework | Arduino via PlatformIO |
| Main library | SensESP 3.2.0 |

---

## Quick Links

- [SensESP documentation](https://signalk.github.io/SensESP/)
- [SignalK specification](https://signalk.org/specification/1.5.0/doc/)
- [TFmini-S product page](https://www.benewake.com/en/tfminis.html)
- [U8g2 library](https://github.com/olikraus/u8g2)
- [PlatformIO documentation](https://docs.platformio.org/)
