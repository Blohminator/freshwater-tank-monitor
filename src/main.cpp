#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <memory>

#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp_app_builder.h"
#include "sensesp/ui/config_item.h"
#include "sensesp/sensors/constant_sensor.h"
#include "sensesp/transforms/lambda_transform.h"
#include "math.h"


using namespace sensesp;

// ================= Hardware =================
#define RXD2 16
#define TXD2 17
#define ALARM_PIN 23

// ================= Display =================
#define OLED_SDA 21
#define OLED_SCL 22
U8G2_SH1106_128X64_NONAME_F_SW_I2C display(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

// ================= TFmini =================
const uint8_t TFMINI_HEADER = 0x59;
const int TFMINI_FRAME_SIZE = 9;

// ================= Config =================
float cfg_length     = 100.0f;
float cfg_width      = 50.0f;
float cfg_height     = 110.0f;
float cfg_offset     = 5.0f;
float cfg_alarm_high = 95.0f;

float tank_capacity_liters() {
  return cfg_length * cfg_width * cfg_height / 1000.0f;
}
float tank_capacity_m3() {
  return tank_capacity_liters() / 1000.0f;
}

// ================= SignalK =================
SKOutputFloat* sk_level;
SKOutputFloat* sk_capacity;
SKOutputFloat* sk_volume;

// ================= Display State =================
int disp_percent = 0;
int disp_height  = 0;

// ================= Display =================
void updateDisplay() {
  display.clearBuffer();

  // Titel
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 12, "Freshwater Tank");

  // Füllstand
  display.setFont(u8g2_font_ncenB10_tr);
  display.setCursor(0, 30);
  display.print("Level: ");
  display.print(disp_percent);
  display.print(" %");

  // Höhe
  display.setCursor(0, 50);
  display.print("Height:");
  display.setCursor(70, 50);
  display.print(disp_height);
  display.print(" cm");

  display.sendBuffer();
}

// ================= TFmini Reader =================
int read_tfmini() {
  static uint8_t buf[TFMINI_FRAME_SIZE * 4];
  static int buf_len = 0;
  int last_valid = -1;

  while (Serial2.available()) {
    if (buf_len >= (int)sizeof(buf)) buf_len = 0;
    buf[buf_len++] = Serial2.read();
  }

  int i = 0;
  while (i <= buf_len - TFMINI_FRAME_SIZE) {
    if (buf[i] != TFMINI_HEADER || buf[i+1] != TFMINI_HEADER) {
      i++; continue;
    }

    uint8_t cs = 0;
    for (int j = i; j < i+8; j++) cs += buf[j];
    if (cs != buf[i+8]) {
      i++; continue;
    }

    last_valid = buf[i+2] + (buf[i+3] << 8);
    i += TFMINI_FRAME_SIZE;
  }

  if (i > 0) {
    buf_len -= i;
    memmove(buf, buf+i, buf_len);
  }

  return last_valid;
}

// ================= Sensor =================
class TFminiSensor : public Sensor<float> {
 public:
  TFminiSensor() : Sensor<float>("") {
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    Serial.println("TFmini gestartet");

    event_loop()->onRepeat(500, [this]() { this->update(); });
  }

 private:
  float filtered_ratio = 0.0f;
  float last_ratio     = -1.0f;
  const float alpha    = 0.1f;

  uint32_t last_send = 0;

  

void update() {
  int dist = read_tfmini();
  Serial.println(dist);   // DEBUG

  if (dist <= 0) {
    return;
  }

  float empty  = constrain((float)(dist - cfg_offset), 0.0f, cfg_height);
  float fill_h = cfg_height - empty;
  float raw_ratio = constrain(fill_h / cfg_height, 0.0f, 1.0f);

  // Glättung
  filtered_ratio = filtered_ratio + alpha * (raw_ratio - filtered_ratio);

  disp_height  = (int)roundf(fill_h);
    disp_height = (int)roundf(filtered_ratio * cfg_height);
    disp_percent = (int)roundf(filtered_ratio * 100.0f);


  Serial.printf("Dist=%d cm | Raw=%.2f | Smooth=%.2f\n",
                dist, raw_ratio * 100, filtered_ratio * 100);

  digitalWrite(ALARM_PIN,
    (disp_percent >= (int)cfg_alarm_high) ? HIGH : LOW);

  updateDisplay();

  if ((millis() - last_send > 2000) &&
      fabs(filtered_ratio - last_ratio) > 0.01) {

    last_send = millis();
    last_ratio = filtered_ratio;

    sk_level->set(filtered_ratio);
    sk_volume->set(tank_capacity_m3() * filtered_ratio);
  }
}
};
// ================= Setup =================
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);

  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 15, "Init...");
  display.sendBuffer();

  SetupLogging();

  SensESPAppBuilder builder;
  sensesp_app = (&builder)
    ->set_hostname("freshwater-tank")
    ->get_app();

  // ===== Web Config =====
  auto* sens_length = new ConstantSensor<float>(cfg_length, 0, "/Tank/Length_cm");
  ConfigItem(sens_length)->set_title("Tank Length (cm)");
  sens_length->connect_to(new LambdaConsumer<float>([](float v){ cfg_length = v; }));

  auto* sens_width = new ConstantSensor<float>(cfg_width, 0, "/Tank/Width_cm");
  ConfigItem(sens_width)->set_title("Tank Width (cm)");
  sens_width->connect_to(new LambdaConsumer<float>([](float v){ cfg_width = v; }));

  auto* sens_height = new ConstantSensor<float>(cfg_height, 0, "/Tank/Height_cm");
  ConfigItem(sens_height)->set_title("Tank Height (cm)");
  sens_height->connect_to(new LambdaConsumer<float>([](float v){ cfg_height = v; }));

  auto* sens_offset = new ConstantSensor<float>(cfg_offset, 0, "/Tank/Offset_cm");
  ConfigItem(sens_offset)->set_title("Sensor Offset (cm)");
  sens_offset->connect_to(new LambdaConsumer<float>([](float v){ cfg_offset = v; }));

  auto* sens_alarm = new ConstantSensor<float>(cfg_alarm_high, 0, "/Tank/Alarm_pct");
  ConfigItem(sens_alarm)->set_title("Alarm Threshold %");
  sens_alarm->connect_to(new LambdaConsumer<float>([](float v){ cfg_alarm_high = v; }));

  // ===== SignalK =====
  sk_level    = new SKOutputFloat("tanks.freshWater.0.currentLevel");
  sk_capacity = new SKOutputFloat("tanks.freshWater.0.capacity");
  sk_volume   = new SKOutputFloat("tanks.freshWater.0.currentVolume");

  event_loop()->onDelay(5000, []() {
    sk_capacity->set(tank_capacity_m3());
  });

  event_loop()->onRepeat(60000, []() {
    sk_capacity->set(tank_capacity_m3());
  });

  new TFminiSensor();

  Serial.println("System ready");
}

// ================= LOOP =================
void loop() {
  event_loop()->tick();
}