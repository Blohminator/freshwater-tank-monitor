# Troubleshooting — Freshwater Tank Monitor

## Quick Diagnosis Checklist

Before diving into specific issues, run through this list:

- [ ] All wiring connections are secure
- [ ] TFmini-S is powered from **5V** (not 3.3V)
- [ ] TX/RX lines are **crossed** (TFmini TX → GPIO 16, TFmini RX → GPIO 17)
- [ ] OLED is powered from **3.3V** (not 5V)
- [ ] Serial monitor is set to **115200 baud**
- [ ] Tank dimensions are configured in the web interface
- [ ] Sensor offset is set correctly

---

## Display Issues

### Display stays completely blank
**Possible causes:**
- Wrong power voltage (5V instead of 3.3V will damage the display)
- Loose SDA/SCL connection
- Wrong I2C address

**Fix:**
1. Verify OLED VCC is connected to **3.3V**, not 5V
2. Check SDA → GPIO 21 and SCL → GPIO 22
3. Run an I2C scanner sketch to confirm address is `0x3C`
4. Check for loose dupont connectors

---

### Display shows "Init..." but never updates
**Possible causes:**
- TFmini-S not sending data
- Serial2 not initialized correctly

**Fix:**
1. Check TFmini-S wiring (TX/RX crossed, 5V power)
2. Open serial monitor — you should see distance values printed every 500ms
3. If distance reads `-1` or `0`, the sensor is not responding (see LiDAR section below)

---

### Display shows garbled pixels or wrong characters
**Possible causes:**
- Wrong display driver (not SH1106)
- I2C interference or too-long wires

**Fix:**
1. Confirm your display uses the **SH1106** chip (not SSD1306 — they look identical but need different drivers)
2. Keep I2C wires short (< 30cm)
3. Add 4.7kΩ pull-up resistors on SDA and SCL to 3.3V if not already on the module

---

## LiDAR / Sensor Issues

### No distance readings (serial shows -1 or 0)
**Possible causes:**
- TX/RX not crossed
- Insufficient 5V power
- Sensor out of range or obstructed

**Fix:**
1. Double-check: **TFmini TX (Green) → GPIO 16**, TFmini RX (White) → GPIO 17
2. Measure voltage at TFmini VCC pin — must be **4.8–5.2V**
3. Ensure sensor has clear line of sight to the water surface
4. Minimum measurement distance is **30cm** — if sensor is too close, readings will be invalid
5. Try powering the ESP32 from a powered USB hub instead of a laptop port

---

### Readings are unstable or jumping
**Possible causes:**
- Surface reflections (water ripples)
- Sensor not mounted perpendicular to water surface
- Electrical noise

**Fix:**
1. The EMA filter (α=0.1) smooths out most noise — this is normal behavior
2. Ensure sensor points **straight down**, not at an angle
3. If tank has strong ripples, consider reducing α in code for more smoothing (lower = smoother but slower response)
4. Keep sensor wiring away from power cables

---

### Readings seem offset (level always too high or too low)
**Possible causes:**
- Sensor offset not configured correctly
- Tank height not set correctly

**Fix:**
1. Measure the exact distance from the **sensor face** to the **top edge of the tank**
2. Enter this value as `Sensor Offset` in the web interface (`/Tank/Offset_cm`)
3. Verify `Tank Height` matches the **internal** height of the tank (not external)
4. With an empty tank, the serial monitor should show: `Dist ≈ cfg_height + cfg_offset`

---

## WiFi / Network Issues

### ESP32 does not create the access point
**Possible causes:**
- Firmware not flashed correctly
- Power issue during boot

**Fix:**
1. Re-flash the firmware: `pio run --target upload`
2. Press the ESP32 reset button after flashing
3. Wait up to 30 seconds for the AP to appear
4. Check serial monitor for boot messages

---

### Cannot connect to `freshwater-tank.local`
**Possible causes:**
- mDNS not supported on your device/network
- Device not yet connected to WiFi

**Fix:**
1. Check serial monitor — it will print the assigned IP address on successful WiFi connection
2. Use the IP address directly instead of the hostname (e.g. `http://192.168.1.42`)
3. On Android, mDNS may not work — always use the IP address
4. Make sure your phone/computer is on the **same WiFi network** as the ESP32

---

### WiFi keeps disconnecting
**Possible causes:**
- Weak signal at sensor location
- Power supply too weak

**Fix:**
1. Move ESP32 closer to the router for testing
2. Use a 1A or stronger USB power supply
3. Check serial monitor for reconnection messages — SensESP handles reconnection automatically

---

## SignalK Issues

### SignalK not receiving any data
**Possible causes:**
- Wrong SignalK server address or port
- SignalK server not running
- Data change threshold not met

**Fix:**
1. Open web interface → verify SignalK server IP and port (default: 3000)
2. Confirm SignalK server is running and accessible from the same network
3. Note: data is only sent when the level changes by **more than 1%** — in a stable tank, updates may be infrequent
4. Check SignalK dashboard for the path `tanks.freshWater.0.currentLevel`

---

### SignalK shows wrong units or values
**Possible causes:**
- Tank dimensions not configured

**Fix:**
1. `currentLevel` is a ratio (0.0–1.0), not a percentage — this is the SignalK standard
2. `capacity` and `currentVolume` are in **m³** — this is the SignalK standard
3. Verify tank dimensions in the web interface are correct

---

## Alarm Issues

### Alarm never triggers
**Possible causes:**
- Wrong GPIO connection
- Alarm threshold set too high

**Fix:**
1. Verify alarm device is connected to **GPIO 23** and GND
2. Check alarm threshold in web interface (`/Tank/Alarm_pct`) — default is 95%
3. Test by temporarily setting threshold to a low value (e.g. 10%) and observing GPIO 23

---

### Alarm triggers at wrong level
**Possible causes:**
- Sensor offset incorrect
- Tank height incorrect

**Fix:**
1. Recalibrate sensor offset (see "Readings seem offset" above)
2. Adjust alarm threshold in web interface

---

## Power Issues

### ESP32 resets randomly
**Possible causes:**
- Power supply too weak
- Brownout during WiFi transmission

**Fix:**
1. Use a **1A minimum** USB power supply (laptop ports often only provide 500mA)
2. Add a 100µF capacitor between 5V and GND near the ESP32
3. Check serial monitor for brownout reset messages

---

## Serial Monitor Debug Output

With serial monitor open at **115200 baud**, you should see:

```
TFmini started
System ready
Dist=85 cm | Raw=72.73 | Smooth=72.50
Dist=85 cm | Raw=72.73 | Smooth=72.52
...
```

- `Dist` = raw distance from sensor in cm
- `Raw` = calculated fill percentage before smoothing
- `Smooth` = EMA-filtered fill percentage (this is what gets sent to SignalK and displayed)

If you see no output at all, the firmware may not have flashed correctly. Try `pio run --target upload` again.

---

## Still stuck?

Check the following resources:
- [SensESP documentation](https://signalk.github.io/SensESP/)
- [TFmini-S datasheet](https://www.benewake.com/en/tfminis.html)
- [SignalK documentation](https://signalk.org/specification/1.5.0/doc/)
- [U8g2 library](https://github.com/olikraus/u8g2)
