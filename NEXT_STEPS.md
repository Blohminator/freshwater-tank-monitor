# Next Steps & Ideas — Freshwater Tank Monitor

This document collects potential improvements, feature ideas, and known limitations for future development.

---

## High Priority

### Persistent configuration storage
Currently, tank parameters configured via the web interface are stored in SensESP's internal storage (SPIFFS/NVS). Verify that values survive a power cycle and document the behavior clearly.

### Sensor health monitoring
Add detection for sensor failure states:
- No data received for > 5 seconds → display warning, send SignalK alert
- Readings outside plausible range (< 0 or > cfg_height + cfg_offset + margin) → flag as invalid
- Currently invalid readings are silently ignored (`if (dist <= 0) return`)

### Low-level alarm
Currently only an overfill (high) alarm exists. A low-level alarm (e.g. tank below 10%) would be useful for freshwater monitoring — warn before the tank runs empty.

---

## Medium Priority

### Two alarm thresholds
- `alarm_high` — overfill warning (already implemented)
- `alarm_low` — low water warning (not yet implemented)
- Both configurable via web interface

### Display improvements
- Show tank volume in liters on display (currently only % and cm)
- Cycle between screens (level view / volume view) on a timer
- Show WiFi/SignalK connection status icon

### Configurable EMA filter strength
Currently α = 0.1 is hardcoded. Exposing this as a web-configurable parameter would allow tuning for different tank geometries and water surface conditions.

### Non-rectangular tank support
Current volume calculation assumes a rectangular tank:
```
Volume = length × width × height × fill_ratio
```
For cylindrical or irregular tanks, a lookup table or different formula would be needed.

---

## Low Priority / Ideas

### OTA firmware updates
SensESP supports OTA (Over-The-Air) updates. Document and enable this so the device can be updated without physical access — useful when the sensor is mounted in a hard-to-reach location.

### Historical data logging
Log fill level readings to SPIFFS with timestamps. Useful for tracking consumption patterns over time. Could be exposed via a simple HTTP endpoint.

### Multiple tank support
Run two instances on separate ESP32 boards (e.g. freshwater + greywater) and display both in SignalK. Each device gets its own hostname and SignalK path.

### Battery backup / low-power mode
For installations without permanent 5V power, investigate deep sleep between readings to extend battery life.

### Physical reset button
Add a button (e.g. on GPIO 0 / BOOT) to reset WiFi credentials without reflashing — useful during installation or network changes.

---

## Known Limitations

| Limitation | Details |
|------------|---------|
| Minimum sensor distance | TFmini-S minimum range is 30cm — tanks shallower than ~25cm may not work reliably |
| Rectangular tanks only | Volume calculation assumes a box-shaped tank |
| Single alarm output | Only one GPIO alarm output, only high-level threshold |
| No offline data | No local data logging — all data is lost if SignalK is unreachable |
| mDNS on Android | `freshwater-tank.local` may not resolve on Android devices — use IP address instead |
| EMA lag | The smoothing filter introduces a small lag (~5–10 readings) when the level changes quickly |
