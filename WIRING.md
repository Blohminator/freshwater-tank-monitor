# Wiring Diagram — Freshwater Tank Monitor

## Complete Wiring Overview

```
                                    ESP32 Development Board
                                   ┌─────────────────────┐
                                   │                     │
    TFmini-S LiDAR                 │   GPIO 16 (RXD2) ◄──┼── TX (Green)
    ┌──────────────┐               │   GPIO 17 (TXD2) ──►┼── RX (White)
    │              │               │                     │
    │   TFmini-S   │               │   5V ───────────────┼── VCC (Red)
    │              │               │   GND ──────────────┼── GND (Black)
    └──────────────┘               │                     │
                                   │                     │
    OLED Display (I2C)             │   GPIO 21 (SDA) ────┼── SDA
    ┌──────────────┐               │   GPIO 22 (SCL) ────┼── SCL
    │  ┌────────┐  │               │   3.3V ─────────────┼── VCC
    │  │128x64  │  │               │   GND ──────────────┼── GND
    │  │SH1106  │  │               │                     │
    │  └────────┘  │               │                     │
    └──────────────┘               │   GPIO 23 ──────────┼── Alarm Output
                                   │                     │
    Alarm Device                   │   5V (USB) ─────────┼── Power Supply
    ┌──────────────┐               │   GND ──────────────┼── Ground
    │   Buzzer/    │               │                     │
    │   Relay/LED  │               └─────────────────────┘
    └──────────────┘
```

---

## Pin Assignment

| Function | ESP32 GPIO | Direction | Connected To | Wire Color |
|----------|------------|-----------|--------------|------------|
| LiDAR RX | GPIO 16 (RXD2) | Input | TFmini-S TX | Green |
| LiDAR TX | GPIO 17 (TXD2) | Output | TFmini-S RX | White |
| I2C Data | GPIO 21 (SDA) | Bidirectional | OLED SDA | — |
| I2C Clock | GPIO 22 (SCL) | Output | OLED SCL | — |
| Alarm Output | GPIO 23 | Output | Alarm Device | — |
| Power 5V | 5V / VIN | Power | TFmini-S VCC | Red |
| Power 3.3V | 3.3V | Power | OLED VCC | — |
| Ground | GND | Ground | All GND | Black |

---

## TFmini-S LiDAR Sensor

**Connector:** 4-pin JST

| Pin | Function | Color | Connect to ESP32 |
|-----|----------|-------|------------------|
| 1 | VCC (5V) | Red | 5V or VIN |
| 2 | TX (Data Out) | Green | GPIO 16 (RXD2) |
| 3 | RX (Data In) | White | GPIO 17 (TXD2) |
| 4 | GND | Black | GND |

> ⚠️ TX and RX are **crossed**: TFmini TX → ESP32 RX, TFmini RX → ESP32 TX
> ⚠️ Requires **5V** power — 3.3V is not sufficient
> ⚠️ Ensure stable 5V supply (minimum 100mA)

---

## OLED Display (SH1106, 128x64, I2C)

**Interface:** I2C — address `0x3C`

| Pin | Function | Connect to ESP32 |
|-----|----------|------------------|
| VCC | Power | 3.3V |
| GND | Ground | GND |
| SCL | I2C Clock | GPIO 22 |
| SDA | I2C Data | GPIO 21 |

> ✓ Uses **3.3V** — do not connect to 5V
> ✓ I2C address: `0x3C` (default)
> ✓ Pull-up resistors are usually included on the display module

---

## Alarm Output (Optional)

**GPIO 23** can drive the following devices:

| Device Type | Connection | Notes |
|-------------|------------|-------|
| LED | GPIO 23 → 220Ω resistor → LED → GND | Direct connection |
| Active Buzzer | GPIO 23 → Buzzer+ / Buzzer− → GND | Use 3.3V compatible buzzer |
| Relay Module | GPIO 23 → Relay IN / VCC → 3.3V / GND → GND | Use 3.3V relay module |
| NPN Transistor | GPIO 23 → Base (via 1kΩ) → NPN transistor | For higher current loads |

> ⚠️ GPIO 23 outputs 3.3V logic — do not connect directly to 5V devices

---

## Power Supply

### Option 1: USB Power (Recommended)
```
USB Cable (5V, min. 500mA)
    │
    └──► ESP32 USB Port
         │
         ├──► 5V Rail → TFmini-S VCC
         ├──► 3.3V Rail (onboard regulator) → OLED VCC
         └──► GND → All devices
```

### Option 2: External 5V Supply
```
5V Power Supply (min. 1A)
    │
    ├──► ESP32 VIN Pin
    ├──► TFmini-S VCC
    └──► GND → All devices
         │
         └──► OLED VCC via ESP32 3.3V pin
```

### Power Requirements

| Component | Current Draw |
|-----------|-------------|
| ESP32 | 80–260 mA (depends on WiFi activity) |
| TFmini-S | 100–150 mA |
| OLED Display | 20–30 mA |
| **Total** | **200–450 mA** |

Use a **1A or higher** power supply for a safe margin.

---

## Assembly Steps

### Step 1 — Prepare components
1. Gather all components
2. Inspect ESP32 board for damage
3. Test OLED display separately if possible

### Step 2 — Connect OLED display
1. Connect VCC to ESP32 **3.3V**
2. Connect GND to ESP32 GND
3. Connect SCL to GPIO 22
4. Connect SDA to GPIO 21

### Step 3 — Connect TFmini-S LiDAR
1. ⚠️ **Cross the TX/RX connections!**
2. Connect TFmini TX (Green) to ESP32 **GPIO 16** (RXD2)
3. Connect TFmini RX (White) to ESP32 **GPIO 17** (TXD2)
4. Connect TFmini VCC (Red) to ESP32 **5V**
5. Connect TFmini GND (Black) to ESP32 GND

### Step 4 — Connect alarm output (optional)
1. Connect alarm device input to GPIO 23
2. Connect alarm device ground to ESP32 GND
3. If using a relay module, connect relay VCC to 3.3V or 5V as required by the module

### Step 5 — Power connection
1. Connect USB cable to ESP32, or
2. Connect external 5V supply to VIN and GND

### Step 6 — Test
1. Power on the system
2. OLED should show **"Init..."** briefly, then live readings
3. Verify TFmini-S LED is on
4. Open serial monitor at 115200 baud — distance values should appear every 500ms

---

## Troubleshooting Wiring Issues

| Problem | Possible Cause | Solution |
|---------|----------------|----------|
| Display stays blank | Wrong power voltage | Check 3.3V connection — not 5V |
| Display shows garbage | Wrong I2C address or wrong chip | Verify SH1106 chip, confirm address 0x3C |
| No LiDAR readings | TX/RX not crossed | Swap GPIO 16 and 17 connections |
| LiDAR not working | Insufficient power | Check 5V supply, use powered USB hub |
| System resets randomly | Power supply too weak | Use 1A or higher power supply |
| Alarm not triggering | Wrong GPIO or threshold | Verify GPIO 23 connection and threshold in web interface |

---

## Safety Notes

1. **Voltage levels**
   - OLED Display: **3.3V only** — 5V will damage it
   - TFmini-S: **5V only** — 3.3V is insufficient
   - ESP32 GPIOs: 3.3V logic level

2. **Current protection**
   - Do not exceed GPIO current limits (12mA per pin)
   - Use a fused USB cable or add an inline fuse (1A) for permanent installations

3. **ESD protection**
   - Handle ESP32 carefully — it is static sensitive
   - Ground yourself before touching components

4. **Mounting**
   - Keep the LiDAR sensor away from water splashes
   - Ensure adequate ventilation around the ESP32
   - Use non-conductive mounting materials

---

## Testing Checklist

- [ ] All connections secure
- [ ] TFmini-S powered from 5V
- [ ] OLED powered from 3.3V
- [ ] TX/RX crossed correctly
- [ ] I2C pull-ups present (on module or external)
- [ ] Power supply rated at 1A or higher
- [ ] No short circuits
- [ ] Display shows "Init..." then live data
- [ ] LiDAR readings visible in serial monitor
- [ ] WiFi connects successfully
- [ ] Web interface accessible at `freshwater-tank.local`
- [ ] Tank parameters configured via web interface
- [ ] SignalK receives data
