# Quickstart Guide — Freshwater Tank Monitor

## What you need
- ESP32 Dev Board (ESP32-WROOM-32)
- TFmini-S LiDAR sensor
- SH1106 OLED Display (128x64, I2C)
- USB cable (data + power)
- Your WiFi credentials
- A running SignalK server (optional)

---

## Step 1 — Wire it up

| TFmini-S | ESP32 |
|----------|-------|
| TX (Green) | GPIO 16 (RXD2) |
| RX (White) | GPIO 17 (TXD2) |
| VCC (Red) | 5V |
| GND (Black) | GND |

| OLED | ESP32 |
|------|-------|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 3.3V |
| GND | GND |

| Alarm (optional) | ESP32 |
|------------------|-------|
| Signal | GPIO 23 |
| GND | GND |

> ⚠️ TFmini-S needs **5V**, not 3.3V!
> ⚠️ TX/RX are **crossed**: TFmini TX → ESP32 GPIO 16

See [WIRING.md](WIRING.md) for the full wiring diagram.

---

## Step 2 — Flash the firmware

```bash
pio run --target upload
```

---

## Step 3 — First boot WiFi setup

1. ESP32 creates access point: **`SensESP-freshwater-tank`**
2. Connect with password: **`thisisfine`**
3. Open browser → `192.168.4.1`
4. Enter your WiFi SSID + password
5. Enter your SignalK server IP + port
6. Save → device reboots and connects to your network

---

## Step 4 — Configure tank dimensions

Open the web interface at **`http://freshwater-tank.local`**

Navigate to the config items and set your values:

| Parameter | Path | Example |
|-----------|------|---------|
| Tank Length | `/Tank/Length_cm` | 100 |
| Tank Width | `/Tank/Width_cm` | 50 |
| Tank Height | `/Tank/Height_cm` | 110 |
| Sensor Offset | `/Tank/Offset_cm` | 5 |
| Alarm Threshold | `/Tank/Alarm_pct` | 95 |

> 💡 **Sensor Offset** = distance from the sensor face to the top edge of the tank (in cm).
> Measure this carefully — it directly affects all level calculations.

Changes take effect immediately without restarting the device.

---

## Step 5 — Verify

- OLED shows **"Freshwater Tank"** with live level % and height in cm
- Serial monitor at 115200 baud shows distance readings and smoothed values
- SignalK receives `tanks.freshWater.0.currentLevel`, `capacity`, and `currentVolume`
- Alarm output (GPIO 23) goes HIGH when level reaches the configured threshold

---

## Done ✓

For more details see:
- [README.md](README.md) — full documentation
- [WIRING.md](WIRING.md) — detailed wiring
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) — common problems and fixes
