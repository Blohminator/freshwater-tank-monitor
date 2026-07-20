/*============================================
  Freshwater Tank Level Monitor
  Hardware:  ESP32-WROOM-32 + TFmini-S LiDAR
  Framework: SensESP 3.2.0 -> Signal K
  Display:   OLED SH1106 128x64 (Software I2C)
  Features:  Web-configurable tank params, EMA filter, alarm output
============================================*/

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <memory>

#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp_app_builder.h"
#include "sensesp/signalk/signalk_metadata.h"
#include "sensesp/ui/config_item.h"
#include "sensesp/sensors/constant_sensor.h"
#include "sensesp/transforms/lambda_transform.h"
#include "math.h"

using namespace sensesp;

// ============================================================
// PIN ASSIGNMENTS
// ============================================================
#define RXD2      16   // ESP32 RX2  <- TFmini-S TX
#define TXD2      17   // ESP32 TX2  -> TFmini-S RX
#define ALARM_PIN 23   // Alarm output (active-high)

// ============================================================
// OLED DISPLAY (SH1106, Software I2C)
// ============================================================
#define OLED_SDA 21
#define OLED_SCL 22
U8G2_SH1106_128X64_NONAME_F_SW_I2C display(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

// ============================================================
// TFmini-S PROTOCOL CONSTANTS
// ============================================================
const uint8_t TFMINI_HEADER     = 0x59;  // Frame start byte (appears twice)
const int     TFMINI_FRAME_SIZE = 9;     // Total bytes per frame

// ============================================================
// TANK CONFIGURATION
// All values are configurable via the SensESP web UI at
// http://freshwater-tank.local and are persisted in flash.
// ============================================================
float cfg_length     = 100.0f;  // Tank internal length (cm)
float cfg_width      =  50.0f;  // Tank internal width (cm)
float cfg_height     = 110.0f;  // Tank internal height (cm)
float cfg_dist_empty = 115.0f;  // Sensor reading when tank is EMPTY (cm)
float cfg_dist_full  =   2.0f;  // Sensor reading when tank is FULL (cm)
float cfg_alarm_high =  95.0f;  // Alarm ON threshold (% fill level)

// cfg_dist_empty / cfg_dist_full calibration guide:
//   1. Empty the tank completely → read "Dist=" from serial monitor → set cfg_dist_empty
//   2. Fill the tank completely  → read "Dist=" from serial monitor → set cfg_dist_full
//   This method handles any sensor mounting height and accounts for
//   channels/pipes between sensor and tank that may fill with water.

// ============================================================
// TANK GEOMETRY HELPERS
// ============================================================
/** Returns total tank capacity in litres based on current config. */
float tank_capacity_liters() {
  return cfg_length * cfg_width * cfg_height / 1000.0f;
}

/** Returns total tank capacity in cubic metres based on current config. */
float tank_capacity_m3() {
  return tank_capacity_liters() / 1000.0f;
}

// ============================================================
// SIGNAL K OUTPUT HANDLES
// Initialised in setup(), written from TFminiSensor::update().
// ============================================================
SKOutputFloat* sk_level;     // tanks.freshWater.0.currentLevel  [ratio 0..1]
SKOutputFloat* sk_capacity;  // tanks.freshWater.0.capacity       [m³]
SKOutputFloat* sk_volume;    // tanks.freshWater.0.currentVolume  [m³]

// ============================================================
// SHARED DISPLAY STATE
// Written by TFminiSensor::update(), read by updateDisplay().
// ============================================================
int   disp_percent  = 0;
float disp_height_cm = 0.0f;
bool  display_ok    = false;  // true after display initialised successfully

// ============================================================
// DISPLAY UPDATE
// Renders the current state to the OLED buffer and flushes it.
// ============================================================
void updateDisplay() {
  if (!display_ok) return;

  display.clearBuffer();

  // Title
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 12, "Frischwassertank");

  // Fill level
  display.setFont(u8g2_font_ncenB10_tr);
  display.setCursor(0, 30);
  display.print("Level: ");
  display.print(disp_percent);
  display.print(" %");

  // Fill height
  display.setCursor(0, 50);
  display.print("Hoehe: ");
  display.setCursor(70, 50);
  display.print((int)disp_height_cm);
  display.print(" cm");

  display.sendBuffer();
}

// ============================================================
// TFmini-S FRAME PARSER
// Drains Serial2, validates frames (dual-header + checksum) and
// returns the distance value [cm] of the last valid frame.
// Returns -1 if no valid frame was received since the last call.
// ============================================================
int read_tfmini() {
  static uint8_t buf[TFMINI_FRAME_SIZE * 4];  // Ring buffer for incoming bytes
  static int buf_len = 0;
  int last_valid = -1;

  // Append all available bytes to the buffer
  while (Serial2.available()) {
    if (buf_len >= (int)sizeof(buf)) buf_len = 0;  // Overflow guard: reset
    buf[buf_len++] = Serial2.read();
  }

  // Scan buffer for complete, valid frames
  int i = 0;
  while (i <= buf_len - TFMINI_FRAME_SIZE) {
    // Check dual-header bytes
    if (buf[i] != TFMINI_HEADER || buf[i+1] != TFMINI_HEADER) {
      i++; continue;
    }

    // Verify checksum (sum of bytes 0..7 == byte 8)
    uint8_t cs = 0;
    for (int j = i; j < i + 8; j++) cs += buf[j];
    if (cs != buf[i+8]) { i++; continue; }

    // Extract 16-bit distance (little-endian, bytes 2 and 3)
    last_valid = buf[i+2] + (buf[i+3] << 8);
    i += TFMINI_FRAME_SIZE;
  }

  // Discard consumed bytes, keep any partial frame at the front
  if (i > 0) {
    buf_len -= i;
    memmove(buf, buf + i, buf_len);
  }

  return last_valid;
}

// ============================================================
// TFminiSensor CLASS
// SensESP sensor that polls the TFmini-S every 100 ms via the
// ReactESP event loop.  On each tick it:
//   1. Reads the latest distance measurement.
//   2. Applies EMA smoothing filter (alpha = 0.1).
//   3. Converts distance to fill height, ratio and volume.
//   4. Publishes values to Signal K.
//   5. Controls the alarm output.
//   6. Refreshes the OLED display.
// ============================================================
class TFminiSensor : public Sensor<float> {
 public:
  TFminiSensor() : Sensor<float>("") {
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    Serial.println("TFmini-S UART started");

    event_loop()->onRepeat(100, [this]() { this->update(); });
  }

 private:
  float filtered_ratio = -1.0f;  // -1 = not yet initialised
  const float alpha    = 0.1f;   // EMA smoothing factor (0=no update, 1=no filter)

  void update() {
    int dist = read_tfmini();  // Raw distance from sensor face to water surface [cm]

    if (dist <= 0) {
      return;
    }

    // Linear interpolation between calibrated empty and full distances.
    // cfg_dist_empty = sensor reading when tank is 0% full (large distance)
    // cfg_dist_full  = sensor reading when tank is 100% full (small distance)
    //
    // As the tank fills, dist decreases from cfg_dist_empty toward cfg_dist_full.
    // This approach works regardless of sensor mounting height and handles
    // channels between sensor and tank that may partially fill with water.
    float span      = cfg_dist_empty - cfg_dist_full;  // total measurable range
    float raw_ratio = constrain((cfg_dist_empty - (float)dist) / span, 0.0f, 1.0f);
    float fill_h    = raw_ratio * cfg_height;

    // Seed the EMA filter on first valid reading to avoid slow convergence
    if (filtered_ratio < 0.0f) {
      filtered_ratio = raw_ratio;
    }

    // Apply exponential moving average filter
    filtered_ratio = filtered_ratio + alpha * (raw_ratio - filtered_ratio);

    // Update shared display state
    disp_height_cm = filtered_ratio * cfg_height;
    disp_percent   = (int)roundf(filtered_ratio * 100.0f);

    Serial.printf("Dist=%d cm | Fill=%.1f cm (%d%%) | Raw=%.1f%% | Smooth=%.1f%%\n",
                  dist, disp_height_cm, disp_percent,
                  raw_ratio * 100.0f, filtered_ratio * 100.0f);

    // Alarm output: HIGH when fill level reaches or exceeds threshold
    digitalWrite(ALARM_PIN, (disp_percent >= (int)cfg_alarm_high) ? HIGH : LOW);

    // Publish to Signal K
    sk_level->set(filtered_ratio);
    sk_volume->set(tank_capacity_m3() * filtered_ratio);

    updateDisplay();
  }
};

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Freshwater Tank Monitor - TFmini-S");

  // --- GPIO initialisation ---
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);  // Alarm off at boot

  // --- Display: first init before SensESP ---
  display.begin();
  display.setContrast(255);
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 15, "Init...");
  display.sendBuffer();
  display_ok = true;
  Serial.println("Display OK");

  SetupLogging();

  // --- SensESP application ---
  SensESPAppBuilder builder;
  sensesp_app = (&builder)
    ->set_hostname("freshwater-tank")
    ->get_app();

  // --- Display: re-init after SensESP ---
  // SensESP may reconfigure the I2C bus during get_app(); reinitialise
  // the display to restore a known good state.
  if (display_ok) {
    display.begin();
    display.setContrast(255);
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 15, "Init...");
    display.sendBuffer();
  }

  // ============================================================
  // TANK CONFIGURATION – WEB UI
  // Each ConstantSensor exposes a field in the SensESP web UI at
  // http://freshwater-tank.local.  The LambdaConsumer writes the
  // value back to the corresponding global config variable so that
  // changes take effect immediately without a reboot.
  // ============================================================
  auto* sens_length = new ConstantSensor<float>(cfg_length, 0, "/Tank/Length_cm");
  ConfigItem(sens_length)
    ->set_title("Tank Length (cm)")
    ->set_description("Internal tank length in cm")
    ->set_sort_order(100);
  sens_length->connect_to(new LambdaConsumer<float>([](float v){ cfg_length = v; }));

  auto* sens_width = new ConstantSensor<float>(cfg_width, 0, "/Tank/Width_cm");
  ConfigItem(sens_width)
    ->set_title("Tank Width (cm)")
    ->set_description("Internal tank width in cm")
    ->set_sort_order(200);
  sens_width->connect_to(new LambdaConsumer<float>([](float v){ cfg_width = v; }));

  auto* sens_height = new ConstantSensor<float>(cfg_height, 0, "/Tank/Height_cm");
  ConfigItem(sens_height)
    ->set_title("Tank Height (cm)")
    ->set_description("Internal tank height in cm")
    ->set_sort_order(300);
  sens_height->connect_to(new LambdaConsumer<float>([](float v){ cfg_height = v; }));

  auto* sens_dist_empty = new ConstantSensor<float>(cfg_dist_empty, 0, "/Tank/Dist_Empty_cm");
  ConfigItem(sens_dist_empty)
    ->set_title("Sensor reading EMPTY tank (cm)")
    ->set_description("Distance the sensor measures when the tank is completely empty. Read from serial monitor: Dist=XX")
    ->set_sort_order(400);
  sens_dist_empty->connect_to(new LambdaConsumer<float>([](float v){ cfg_dist_empty = v; }));

  auto* sens_dist_full = new ConstantSensor<float>(cfg_dist_full, 0, "/Tank/Dist_Full_cm");
  ConfigItem(sens_dist_full)
    ->set_title("Sensor reading FULL tank (cm)")
    ->set_description("Distance the sensor measures when the tank is completely full. Read from serial monitor: Dist=XX")
    ->set_sort_order(500);
  sens_dist_full->connect_to(new LambdaConsumer<float>([](float v){ cfg_dist_full = v; }));

  auto* sens_alarm = new ConstantSensor<float>(cfg_alarm_high, 0, "/Tank/Alarm_pct");
  ConfigItem(sens_alarm)
    ->set_title("Alarm threshold % (default: 95)")
    ->set_description("Fill level % at which the alarm output is activated")
    ->set_sort_order(600);
  sens_alarm->connect_to(new LambdaConsumer<float>([](float v){ cfg_alarm_high = v; }));

  // ============================================================
  // SIGNAL K OUTPUTS
  // ============================================================
  sk_level    = new SKOutputFloat("tanks.freshWater.0.currentLevel",   "/Tank/Level",
                  new SKMetadata("ratio", "Freshwater tank fill level"));
  sk_capacity = new SKOutputFloat("tanks.freshWater.0.capacity",        "/Tank/Capacity",
                  new SKMetadata("m3",    "Freshwater tank total capacity"));
  sk_volume   = new SKOutputFloat("tanks.freshWater.0.currentVolume",   "/Tank/Volume",
                  new SKMetadata("m3",    "Freshwater tank current volume"));

  // Publish capacity once after 5 s (Signal K server may not be ready at boot)
  event_loop()->onDelay(5000, []() {
    sk_capacity->set(tank_capacity_m3());
  });

  // Re-publish capacity every 60 s to keep the Signal K value fresh
  event_loop()->onRepeat(60000, []() {
    sk_capacity->set(tank_capacity_m3());
  });

  // Start sensor – registers the 100 ms onRepeat callback in the event loop
  new TFminiSensor();

  Serial.println("Setup complete!");
}

// ============================================================
// LOOP
// Delegates all work to the ReactESP event loop.
// Application logic runs exclusively via onRepeat / onDelay
// callbacks registered during setup().
// ============================================================
void loop() {
  event_loop()->tick();
}
