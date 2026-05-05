# Freshwater Tank Level Monitor

ESP32-based tank level monitoring system with TFmini-S LiDAR sensor, OLED display, and SignalK integration.

## Features

- **LiDAR Distance Measurement**: TFmini-S sensor for accurate non-contact level measurement
- **OLED Display**: Real-time display of fill level (%) and height (cm) on 128x64 SH1106 display
- **SignalK Integration**: Full integration with SignalK marine data system via SensESP 3.2.0
- **Web-Configurable Tank Dimensions**: Tank size, sensor offset, and alarm threshold are configurable via the web interface — no recompilation needed
- **Overfill Alarm**: Configurable alarm output when water level exceeds threshold (default 95%)
- **Smoothing Filter**: Exponential moving average (EMA, α=0.1) for stable readings
- **WiFi Connectivity**: Web interface for configuration and monitoring

## Hardware Requirements

### Components
- ESP32 Development Board (ESP32-WROOM-32)
- TFmini-S LiDAR Distance Sensor
- 1.3" OLED Display (128x64, SH1106 chip, I2C)
- Alarm output device (optional, connected to GPIO 23)

### Wiring

#### TFmini-S LiDAR Sensor
| TFmini-S | ESP32 |
|----------|-------|
| TX (Green) | GPIO 16 (RXD2) |
| RX (White) | GPIO 17 (TXD2) |
| VCC (Red) | 5V |
| GND (Black) | GND |

#### OLED Display (I2C)
| OLED | ESP32 |
|------|-------|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 3.3V |
| GND | GND |

#### Alarm Output
| Component | ESP32 |
|-----------|-------|
| Alarm Output | GPIO 23 |

## Software Setup

### Prerequisites
- PlatformIO IDE or PlatformIO Core
- USB cable for ESP32 programming

### Installation

1. Clone or download this project
2. Open the project in PlatformIO
3. Build and upload:
```bash
pio run --target upload
```

4. Configure tank dimensions via the web interface after first boot (see below)

### First Boot Configuration

1. After first boot, the ESP32 creates a WiFi access point named "SensESP-freshwater-tank"
2. Connect to this network with your phone or computer
   - **WiFi Password:** `thisisfine`
3. A captive portal opens automatically (or navigate to 192.168.4.1)
4. Configure your WiFi credentials and SignalK server settings
5. The device will restart and connect to your network

## Tank Configuration (Web Interface)

All tank parameters are configurable via the web interface at `http://freshwater-tank.local` — no code changes required.

> 🔒 **Login required** — Username: `admin` / Password: `thisisfine`

| Parameter | Path | Default | Description |
|-----------|------|---------|-------------|
| Tank Length | `/Tank/Length_cm` | 100 cm | Tank length in cm |
| Tank Width | `/Tank/Width_cm` | 50 cm | Tank width in cm |
| Tank Height | `/Tank/Height_cm` | 110 cm | Tank height in cm |
| Sensor Offset | `/Tank/Offset_cm` | 5 cm | Distance from sensor to tank top |
| Alarm Threshold | `/Tank/Alarm_pct` | 95 % | Fill level that triggers the alarm |

Changes take effect immediately without restarting the device.

## SignalK Integration

The system publishes three values to SignalK:

| Path | Description | Unit |
|------|-------------|------|
| `tanks.freshWater.0.currentLevel` | Current fill level | 0.0–1.0 (ratio) |
| `tanks.freshWater.0.capacity` | Tank capacity | m³ |
| `tanks.freshWater.0.currentVolume` | Current volume | m³ |

`currentLevel` and `currentVolume` are sent when the value changes by more than 1% and at most every 2 seconds.  
`capacity` is sent once after startup (5 s delay) and then every 60 seconds.

## Display Information

The OLED display shows:
- **Line 1**: Title — "Freshwater Tank"
- **Line 2**: Fill level percentage — e.g. "Level: 75 %"
- **Line 3**: Smoothed fill height in centimeters — e.g. "Height: 82 cm"

## Alarm Function

- Alarm output (GPIO 23) goes HIGH when fill level reaches or exceeds the configured threshold
- Default threshold: 95% (overfill warning when filling the tank)
- Configurable via web interface at `/Tank/Alarm_pct`

## Tank Volume Calculation

```
Capacity (liters) = (length_cm × width_cm × height_cm) / 1000
Capacity (m³)     = Capacity (liters) / 1000
Current Volume    = Capacity (m³) × Fill Level Ratio
```

## Smoothing Filter

Raw distance readings are smoothed using an exponential moving average (EMA):

```
filtered = filtered + α × (raw − filtered)   (α = 0.1)
```

This reduces noise from surface ripples or sensor jitter. The displayed height and percentage are both derived from the smoothed value.

## Troubleshooting

### Display shows "Init..." and doesn't update
- Check I2C connections (SDA/SCL)
- Verify display address is 0x3C
- Check if display is SH1106 compatible

### No LiDAR readings
- Verify UART connections (TX/RX are crossed)
- Check 5V power supply to sensor
- Ensure sensor has clear line of sight to water surface

### SignalK not receiving data
- Verify WiFi connection
- Check SignalK server address in web interface
- Ensure SignalK server is running and accessible

### Incorrect volume calculations
- Verify tank dimensions in the web interface
- Check the Sensor Offset value
- Ensure sensor is mounted correctly above tank

### Alarm triggers at wrong level
- Adjust the alarm threshold in the web interface (`/Tank/Alarm_pct`, default 95%)
- Alarm triggers when level >= threshold

## Technical Specifications

- **Measurement Range**: 0.3 m – 12 m (TFmini-S)
- **Measurement Accuracy**: ±6 cm @ 6 m
- **Update Rate**: 2 Hz (500 ms)
- **SignalK Send Interval**: max. every 2 s, only on >1% change
- **Operating Voltage**: 5V (via USB or external power)
- **WiFi**: 802.11 b/g/n (2.4 GHz)
- **Display**: 128×64 pixels, monochrome

## Mounting Notes

1. **Sensor Position**: Mount the LiDAR sensor directly above the center of the tank
2. **Sensor Orientation**: Sensor must point straight down
3. **Offset**: Measure the distance from the sensor face to the top of the tank and enter it as the Sensor Offset
4. **Obstructions**: Ensure no obstacles between sensor and water surface
5. **Vibration**: Mount the sensor as vibration-free as possible

## License

This project is open source. Feel free to modify and adapt for your needs.

## Credits

- Built with [SensESP](https://github.com/SignalK/SensESP) 3.2.0
- Uses [U8g2](https://github.com/olikraus/u8g2) library for display
- Compatible with [SignalK](https://signalk.org/) marine data standard

## Version History

- **v1.1** - Web-configurable parameters, EMA smoothing filter
  - Tank dimensions and alarm threshold configurable via web interface
  - Exponential moving average filter (α=0.1) for stable readings
  - Display updated to show smoothed height
  - Update rate changed to 2 Hz (500 ms)
- **v1.0** - Initial release with SensESP 3.2.0 support
  - TFmini-S LiDAR integration
  - OLED display support
  - SignalK integration
  - Overfill alarm output (95% threshold)
