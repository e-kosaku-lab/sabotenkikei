// おへやのきろくモニター（ESP32 + AHT20/BMP280 + 0.96インチOLED）
//
// このプログラムは Wi-Fi / Bluetooth を一切使いません（WiFi.h などを読み込んでいません）。
//
// 必要なライブラリ（Arduino IDE の「ライブラリを管理」から入れる）:
//   1. U8g2                (olikraus)
//   2. Adafruit BMP280 Library  ※「依存ライブラリも入れますか？」→「すべてインストール」
//
// ボード設定: ツール → ボード → ESP32 Arduino → 「ESP32 Dev Module」
//
// 配線（I2C）:
//   ESP32 3V3     → センサー VCC / OLED VCC
//   ESP32 GND     → センサー GND / OLED GND
//   ESP32 GPIO21  → センサー SDA / OLED SDA
//   ESP32 GPIO22  → センサー SCL / OLED SCL

#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_BMP280.h>

// 0.96インチ OLED（SSD1306, 128x64, I2Cアドレス 0x3C）
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
Adafruit_BMP280 bmp;

const uint8_t AHT20_ADDR = 0x38;
bool bmpOk = false;

// ---- AHT20（温度・湿度）をライブラリなしで読む ----
void ahtInit() {
  delay(40);
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xBE);
  Wire.write(0x08);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(10);
}

bool ahtRead(float &tempC, float &humi) {
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;  // つながっていない
  delay(80);
  if (Wire.requestFrom((uint8_t)AHT20_ADDR, (uint8_t)7) != 7) return false;
  uint8_t d[7];
  for (int i = 0; i < 7; i++) d[i] = Wire.read();
  if (d[0] & 0x80) return false;  // まだ測定中
  uint32_t rawH = ((uint32_t)d[1] << 12) | ((uint32_t)d[2] << 4) | (d[3] >> 4);
  uint32_t rawT = ((uint32_t)(d[3] & 0x0F) << 16) | ((uint32_t)d[4] << 8) | d[5];
  humi  = rawH * 100.0f / 1048576.0f;
  tempC = rawT * 200.0f / 1048576.0f - 50.0f;
  return true;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA=21, SCL=22

  display.begin();
  display.enableUTF8Print();
  // 日本語フォント（「湿」を含むのは japanese2 以上。japanese1 には入っていない）
  display.setFont(u8g2_font_b12_t_japanese2);

  ahtInit();

  // 気圧センサー BMP280 のI2Cアドレスは 0x76 か 0x77（商品により違う）
  bmpOk = bmp.begin(0x76) || bmp.begin(0x77);
  if (!bmpOk) Serial.println("BMP280 が見つかりません（配線を確認）");
}

void loop() {
  float t = 0, h = 0;
  bool ahtOk = ahtRead(t, h);

  char line[40];
  display.clearBuffer();
  display.setFont(u8g2_font_b12_t_japanese2);

  display.drawUTF8(0, 12, "おへやのきろく");

  if (ahtOk) {
    snprintf(line, sizeof(line), "温度 %.1f C", t);
    display.drawUTF8(0, 28, line);

    snprintf(line, sizeof(line), "湿度 %.0f%%", h);
    display.drawUTF8(0, 44, line);
    const char *mood = (h >= 70.0f) ? "じめじめ" : (h < 40.0f) ? "かんそう" : "ふつう";
    display.drawUTF8(72, 44, mood);
  } else {
    display.drawUTF8(0, 28, "センサーが");
    display.drawUTF8(0, 44, "つながっていません");
  }

  if (bmpOk) {
    snprintf(line, sizeof(line), "気圧 %.0f hPa", bmp.readPressure() / 100.0f);
    display.drawUTF8(0, 60, line);
  } else {
    display.drawUTF8(0, 60, "気圧 ---");
  }

  display.sendBuffer();
  Serial.printf("温度=%.1f 湿度=%.0f\n", t, h);
  delay(1000);
}
