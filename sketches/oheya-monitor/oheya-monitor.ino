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
//
// 画面のようす（128 x 64 ドット）:
//   左がわ … 上から「温度」「湿度」「気圧」、いちばん下に「かいてき」などの言葉
//   右上   … しつどに合わせたマーク（にこにこ顔・水てき・サボテン）

#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_BMP280.h>

// =====================================================================
//  かえてみよう！（ここから下を書きかえると、画面の表示がかわります）
//  書きかえたら、もう一度「書き込み」ボタン（→）をおしてね。
// =====================================================================

// ---- ① 数字をかえてみよう ----
// しつど（%）が この数字 より大きいと「じめじめ」、小さいと「かんそう」になります。
const float JIMEJIME_LINE = 70;  // これ以上で じめじめ
const float KANSOU_LINE   = 40;  // これより下で かんそう

// ---- ② 言葉をかえてみよう ----
// " " の中を、すきな言葉にかえられます。（ひらがな・カタカナがおすすめ。
// むずかしい漢字は、画面に出ないことがあります）
const char *WORD_KAITEKI  = "かいてき";
const char *WORD_JIMEJIME = "じめじめ";
const char *WORD_KANSOU   = "かんそう";

// ---- ③ 0と1で 絵をかえてみよう ----
// 1 が「光るドット」、0 が「光らないドット」です。
// 1行に16こ、16行あります。1 をならべて、すきな絵をかいてみよう。
// （画面では、1ドットを 2x2 の大きさにして出します）

// かいてき: にこにこ顔
const char *MARK_KAITEKI[16] = {
  "0000011111100000",
  "0001100000011000",
  "0010000000000100",
  "0100000000000010",
  "0100110000110010",
  "1000110000110001",
  "1000000000000001",
  "1000000000000001",
  "1001000000001001",
  "1000100000010001",
  "0100011111100010",
  "0100000000000010",
  "0010000000000100",
  "0001100000011000",
  "0000011111100000",
  "0000000000000000",
};

// じめじめ: しょんぼりした 水てき
const char *MARK_JIMEJIME[16] = {
  "0000000110000000",
  "0000001001000000",
  "0000010000100000",
  "0000100000010000",
  "0001000000001000",
  "0010000000000100",
  "0100110000110010",
  "0100110000110010",
  "1000000000000001",
  "1000000000000001",
  "1000011111100001",
  "1000100000010001",
  "0100000000000010",
  "0010000000000100",
  "0001100000011000",
  "0000011111100000",
};

// かんそう: サボテン
const char *MARK_KANSOU[16] = {
  "0000000110000000",
  "0000001111000000",
  "0000001111000000",
  "0110001111000000",
  "0110001111000110",
  "0110001111000110",
  "0110001111000110",
  "0111111111000110",
  "0011111111111110",
  "0000001111111100",
  "0000001111000000",
  "0000001111000000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// =====================================================================
//  かえてみよう ここまで（ここから下は、なれてきたら読んでみよう）
// =====================================================================

// 0.96インチ OLED（SSD1306, 128x64, I2Cアドレス 0x3C）
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
Adafruit_BMP280 bmp;

const uint8_t AHT20_ADDR = 0x38;
bool bmpOk = false;

const int MARK_SIZE  = 16;  // 絵のたて・よこのドット数
const int MARK_SCALE = 2;   // 1ドットを何倍で出すか（2 → 32x32ドット）
const int MARK_X = 128 - MARK_SIZE * MARK_SCALE;  // 右はしにぴったり
const int MARK_Y = 4;

// ---- 0と1の絵を、画面にかく ----
// 文字が '1' のところだけ四角をぬる。行が短くても、そこで止まるので安全。
void drawMark(const char *const mark[], int x, int y) {
  for (int row = 0; row < MARK_SIZE; row++) {
    const char *line = mark[row];
    for (int col = 0; col < MARK_SIZE && line[col] != '\0'; col++) {
      if (line[col] == '1') {
        display.drawBox(x + col * MARK_SCALE, y + row * MARK_SCALE, MARK_SCALE, MARK_SCALE);
      }
    }
  }
}

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

  // はじめの画面（2秒）
  display.clearBuffer();
  display.drawUTF8(0, 26, "おへやの");
  display.drawUTF8(0, 44, "きろく");
  drawMark(MARK_KAITEKI, MARK_X, 16);
  display.sendBuffer();

  ahtInit();

  // 気圧センサー BMP280 のI2Cアドレスは 0x76 か 0x77（商品により違う）
  bmpOk = bmp.begin(0x76) || bmp.begin(0x77);
  if (!bmpOk) Serial.println("BMP280 が見つかりません（配線を確認）");

  delay(2000);
}

void loop() {
  float t = 0, h = 0;
  bool ahtOk = ahtRead(t, h);

  char line[40];
  display.clearBuffer();
  display.setFont(u8g2_font_b12_t_japanese2);

  if (ahtOk) {
    snprintf(line, sizeof(line), "温度 %.1f C", t);
    display.drawUTF8(0, 13, line);

    snprintf(line, sizeof(line), "湿度 %.0f%%", h);
    display.drawUTF8(0, 29, line);

    // しつどで、言葉と絵をえらぶ
    const char *word;
    const char *const *mark;
    if (h >= JIMEJIME_LINE) {
      word = WORD_JIMEJIME;
      mark = MARK_JIMEJIME;
    } else if (h < KANSOU_LINE) {
      word = WORD_KANSOU;
      mark = MARK_KANSOU;
    } else {
      word = WORD_KAITEKI;
      mark = MARK_KAITEKI;
    }
    display.drawUTF8(0, 62, word);
    drawMark(mark, MARK_X, MARK_Y);
  } else {
    display.drawUTF8(0, 13, "センサーが");
    display.drawUTF8(0, 29, "つながって");
    display.drawUTF8(0, 62, "いません");
  }

  if (bmpOk) {
    snprintf(line, sizeof(line), "気圧 %.0f hPa", bmp.readPressure() / 100.0f);
    display.drawUTF8(0, 45, line);
  } else {
    display.drawUTF8(0, 45, "気圧 ---");
  }

  display.sendBuffer();
  Serial.printf("温度=%.1f 湿度=%.0f\n", t, h);
  delay(1000);
}
