# Changelog — Freshwater Tank Monitor

All notable changes to this project are documented here.

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [1.1.0] — 2026-05-05

### Added
- Web-configurable tank parameters via SensESP `ConstantSensor` + `ConfigItem`
  - Tank length, width, height, sensor offset, and alarm threshold
  - All parameters configurable at runtime via `http://freshwater-tank.local`
  - No recompilation required for tank dimension changes
- Exponential moving average (EMA) filter for distance readings (α = 0.1)
  - Reduces noise from water surface ripples and sensor jitter
  - Both display and SignalK output use the smoothed value
- SignalK `capacity` published on startup (5s delay) and every 60 seconds
- SignalK `currentLevel` and `currentVolume` sent only on >1% change, max every 2s

### Changed
- Tank configuration moved from compile-time `TankConfig` struct to runtime web config
- Update rate changed from 5 Hz (200ms) to 2 Hz (500ms)
- Display height now shows smoothed value (derived from `filtered_ratio * cfg_height`)
- Display text updated: title "Freshwater Tank", labels "Level:" and "Height:"

### Removed
- Compile-time `TankConfig` struct
- Hardcoded tank dimension constants

---

## [1.0.0] — 2026-02-01

### Added
- Initial release
- TFmini-S LiDAR sensor integration via UART2 (GPIO 16/17)
- SH1106 128x64 OLED display via software I2C (GPIO 21/22)
- SignalK integration via SensESP 3.2.0
  - `tanks.freshWater.0.currentLevel` (ratio 0.0–1.0)
  - `tanks.freshWater.0.capacity` (m³)
  - `tanks.freshWater.0.currentVolume` (m³)
- Overfill alarm output on GPIO 23 (HIGH when level ≥ threshold)
- Alarm threshold: 95% (hardcoded)
- Tank dimensions: hardcoded in source
- WiFi configuration via SensESP captive portal
- Hostname: `freshwater-tank`
- PlatformIO project with C++17, min_spiffs partition
