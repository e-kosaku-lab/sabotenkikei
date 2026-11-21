// さぼてんきくんの ふみきり（ESP32 + 0.96インチOLED + 赤LED2こ + アンプ・スピーカー）
//
// 「さぼてんきけい」の、追加パックの プログラムです。
// ワイヤーを さわると、ふみきりが「カンカン」と なって、赤いLEDが かわりばんこに 光ります。
// 画面では しゃだんきが おりて、電車が 通ります。たまに、さぼてんきくんや、
// 0と1で かいた 絵（MY_ART）も 通ります。
//
// このプログラムは Wi-Fi / Bluetooth を一切使いません（WiFi.h などを読み込んでいません）。
//
// 必要なライブラリ: U8g2 （olikraus）
// ボード設定: ツール → ボード → esp32 → 「ESP32 Dev Module」
//
// 必要な部品（キットのほかに）:
//   ・赤LED 2こ（それぞれに 220Ω くらいの 抵抗を つなぐ）
//   ・アンプ MAX98357A モジュール（I2S）と、スピーカー
//   ・ワイヤー（さわる場所 用）
//
// 配線:
//   ESP32 3V3     → OLED VDD
//   ESP32 GND     → OLED GND
//   ESP32 GPIO21  → OLED SDA
//   ESP32 GPIO22  → OLED SCK
//   ESP32 GPIO16  → 抵抗 → 赤LED（左）の ＋（足の 長い方）。 LEDの − → GND
//   ESP32 GPIO17  → 抵抗 → 赤LED（右）の ＋（足の 長い方）。 LEDの − → GND
//   ESP32 GPIO26  → アンプ BCLK
//   ESP32 GPIO25  → アンプ LRC
//   ESP32 GPIO18  → アンプ DIN
//   ESP32 3V3 → アンプ VIN（5Vでも 動きます。ページの配線図は 3V3）
//   ESP32 GND     → アンプ GND
//   アンプの 出力 → スピーカー（＋と −）
//   ESP32 GPIO14（D14）→ ワイヤー1（さわる場所①）… ふみきりが はじまる
//   ESP32 GPIO27（D27）→ ワイヤー2（さわる場所②）… さぼてんきくんが 通る
//   ワイヤーの、もう一方の先（金属の部分）を、さわります。
//
// つかいかた:
//   ワイヤー1を ちょん → ふみきりが なる → しゃだんきが おりる → 電車（など）が 通る → あがる
//   ワイヤー2を ちょん → ぜったいに さぼてんきくんが 通る
//   ※ 電源を入れる時は、ワイヤーの先に、さわらないでください。

#include <Wire.h>
#include <U8g2lib.h>
#include <ESP_I2S.h>

// =====================================================================
//  かえてみよう！（ここから下を書きかえると、ふみきりがかわります）
//  書きかえたら、もう一度「書き込み」ボタン（→）をおしてね。
// =====================================================================

// ---- ① 通るものを かえてみよう ----
// 3つの数字を 足して 100 まで。のこりが「電車」です。
const int CACTUS_PERCENT = 20;   // さぼてんきくんが 通る わりあい（％）
const int ART_PERCENT = 15;      // MY_ART（じぶんで かいた絵）が 通る わりあい（％）
const int TRAIN_CARS = 3;        // 電車の しゃりょうの かず（1〜4）。さぼてんきくんや 絵も、この数だけ ならんで 通ります
const int TRAIN_SPEED = 70;      // 電車の はやさ（ドット／びょう）
const int WALK_SPEED = 45;       // さぼてんきくんや 絵の はやさ

// ---- ② 音と 光を かえてみよう ----
const int VOLUME = 4000;                  // 音の 大きさ（0〜20000。大きすぎると ひずみます）
const int BELL_HZ_A = 760;                // 「カン」の 高さ（Hz）。数字が 大きいほど 高い音
const int BELL_HZ_B = 760;                // もうひとつの 「カン」の 高さ。ちがう数字（たとえば 640）にすると 高い音・低い音の「カン」「コン」に
const unsigned long LAMP_MS = 600;        // 光る LED が かわる 間かく（ミリ秒）。「カン」も この間かくで 鳴ります

// ---- ③ しゃだんきを かえてみよう ----
const unsigned long PRE_MS = 1500;        // なりはじめてから しゃだんきが おりはじめるまで
const unsigned long LOWER_MS = 1300;      // おりる 時間
const unsigned long RAISE_MS = 1200;      // あがる 時間

// ---- ④ タッチの ぐあいを かえてみよう ----
// 0.3〜0.9 の間で。反応しすぎる時は 小さく、反応しない時は 大きく。
const float TOUCH_RATIO = 0.6;
const int TOUCH_CONFIRM = 3;
const unsigned long LONG_MS = 1200;
const unsigned long ANIME_MS = 500;       // さぼてんきくんの 絵の きりかえの間かく（ミリ秒）

// ---- ⑤ 0と1で 絵をかえてみよう ----
// 1 が「光るドット」、0 が「光らないドット」です。1行に16こ、16行あります。
// Webの「ドット絵メーカー」で かいた 0と1を、ここに はりつけても いいよ。

// じぶんで かいた 絵（ふみきりを 通ります）。はじめは「ハート」です。
const char *MY_ART[16] = {
  "0000000000000000",
  "0011110000111100",
  "0111111001111110",
  "1111111111111111",
  "1111111111111111",
  "1111111111111111",
  "0111111111111110",
  "0011111111111100",
  "0001111111111000",
  "0000111111110000",
  "0000011111100000",
  "0000001111000000",
  "0000000110000000",
  "0000000000000000",
  "0000000000000000",
  "0000000000000000",
};

// かいてき: にこにこ顔の さぼてんきくん
const char *MARK_KAITEKI[16] = {
  "0000000110000000",
  "0000001111000000",
  "0000011111100000",
  "0110010110100000",
  "0110010110100110",
  "0110011111100110",
  "0110010110100110",
  "0111111001100110",
  "0011111111111110",
  "0000011111111100",
  "0000011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// かいてき（絵2）
const char *MARK_KAITEKI2[16] = {
  "0000000110000000",
  "0000001111000000",
  "0000011111100000",
  "0000010110100110",
  "0110010110100110",
  "0110011111100110",
  "0110010110100110",
  "0110011001111110",
  "0111111111111100",
  "0011111111100000",
  "0000011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// じめじめ: じめじめ顔の さぼてんきくん
const char *MARK_JIMEJIME[16] = {
  "0100000110000000",
  "0011001111000110",
  "0000011111101000",
  "0011010110100000",
  "0110010110101100",
  "0110011111100110",
  "0110010000100110",
  "0111111111100110",
  "0011111111111110",
  "0000011111111100",
  "0000011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// じめじめ（絵2）
const char *MARK_JIMEJIME2[16] = {
  "1000000110000000",
  "0100001111000010",
  "0000011111100100",
  "0110011111100000",
  "0110010110100110",
  "0110011111100110",
  "0110010000100110",
  "0111111111100110",
  "0011111111111110",
  "0000011111111100",
  "0000011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// からから: からからで あつそうな さぼてんきくん
const char *MARK_KARAKARA[16] = {
  "0000000110001000",
  "0010001111000100",
  "0100011111101000",
  "0010011111100100",
  "0100010110100000",
  "0000011111111100",
  "0011111001111110",
  "0111110000100110",
  "0110011111100110",
  "0110011111100110",
  "0110011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// からから（絵2）
const char *MARK_KARAKARA2[16] = {
  "0000000110000100",
  "0100001111001000",
  "0010011111100100",
  "0100011111101000",
  "0010010110100000",
  "0000011111111100",
  "0011111001111110",
  "0111110110100110",
  "0110011111100110",
  "0110011111100110",
  "0110011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// いたい！: タッチして、びっくりした さぼてんきくん（絵1）
const char *MARK_ITAI[16] = {
  "1000000110000001",
  "0100001111000010",
  "0000011111100000",
  "0110010110100000",
  "0110011001100110",
  "0110010110100110",
  "0110011001100110",
  "0111111001100110",
  "0011111111111110",
  "0000011111111100",
  "0000011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// いたい！（絵2）
const char *MARK_ITAI2[16] = {
  "0100000110000010",
  "1000001111000001",
  "0000011111100000",
  "0110010110100000",
  "0110011001100110",
  "0110010110100110",
  "0110011001100110",
  "0111111001100110",
  "0011111111111110",
  "0000011111111100",
  "0000011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// えへへ: なでられて、うれしい さぼてんきくん（絵1）
const char *MARK_TERE[16] = {
  "0001000110001000",
  "0011101111011100",
  "0001011111101000",
  "0110011111100000",
  "0110010110100110",
  "0110011111100110",
  "0110010110100110",
  "0111111001100110",
  "0011111111111110",
  "0000011111111100",
  "0000011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// えへへ（絵2）
const char *MARK_TERE2[16] = {
  "0000000110000000",
  "0001001111001000",
  "0000011111100000",
  "0110011111100000",
  "0110010110100110",
  "0110011111100110",
  "0110010110100110",
  "0111111001100110",
  "0011111111111110",
  "0000011111111100",
  "0000011111100000",
  "0000011111100000",
  "0011111111111100",
  "0001000000001000",
  "0001000000001000",
  "0000111111110000",
};

// =====================================================================
//  かえてみよう ここまで（ここから下は、なれてきたら読んでみよう）
// =====================================================================

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

const int MARK_SIZE = 16;
const int MARK_SCALE = 2;

const int LED_L = 16;
const int LED_R = 17;
const int I2S_BCLK = 26;
const int I2S_LRC = 25;
const int I2S_DIN = 18;

struct TouchPad {
  int pin;
  float base;               // さわっていない時の 値（きじゅん）
  bool down;                // いま さわっているか
  int cnt;                  // ちょうし を みる ための かぞえ
  unsigned long pendMs;     // さわり はじめた とき
  unsigned long downMs;     // さわった ときこく
  bool longFired;
  bool pressedEdge;         // いま さわった
  bool releasedEdge;        // いま はなした
  bool longEdge;            // いま ながく さわった
  unsigned long heldMs;     // はなした とき の、さわっていた 長さ
};

// ESP32 は、さわると 値が 小さくなる
float measureBase(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += touchRead(pin);
    delay(4);
  }
  return sum / 16.0f;
}

void padUpdate(TouchPad &p, unsigned long now) {
  p.pressedEdge = false;
  p.releasedEdge = false;
  p.longEdge = false;
  float v = (float)touchRead(p.pin);
  bool raw = v < p.base * TOUCH_RATIO;
  if (raw == p.down) {
    p.cnt = 0;
    // さわっていない間は、きじゅんを、ゆっくり なおす
    if (!raw && v > p.base * 0.8f) p.base = p.base * 0.98f + v * 0.02f;
  } else {
    if (p.cnt == 0) p.pendMs = now;
    p.cnt++;
    if (p.cnt >= TOUCH_CONFIRM) {  // 3回つづけて 同じ → きまり（ちらつき よけ。かず は TOUCH_CONFIRM）
      p.down = raw;
      p.cnt = 0;
      if (raw) {
        p.pressedEdge = true;
        p.downMs = p.pendMs;
        p.longFired = false;
      } else {
        p.releasedEdge = true;
        p.heldMs = now - p.downMs;
      }
    }
  }
  if (p.down && !p.longFired && now - p.downMs >= LONG_MS) {
    p.longFired = true;
    p.longEdge = true;
  }
}

// ---- 音を出す（MAX98357A アンプ + スピーカー）----
// 音は、「音のたんとう」の仕事（別の仕事）が、ずっと出しつづけます。
// g_freq に、音の高さ（Hz）を入れると、その音が鳴りつづけます。0 を入れると、しずかになります。
// g_bellHz に、音の高さ（Hz）を入れると、かねを「カーン」と 1回 たたいた音が 鳴ります（すぐ 0 にもどります）。
const int SAMPLE_RATE = 16000;
I2SClass i2s;
volatile int g_freq = 0;
volatile int g_bellHz = 0;

void audioTask(void *arg) {
  static int16_t buf[160];
  float phase = 0, amp = 0;
  float bellPhase = 0, bellStep = 0, bellAmp = 0;
  while (true) {
    int bh = g_bellHz;
    if (bh > 0) {                 // かねを たたく
      bellStep = 2.0f * 3.14159265f * bh / SAMPLE_RATE;
      bellPhase = 0;
      bellAmp = 1.0f;
      g_bellHz = 0;
    }
    int f = g_freq;
    float target = (f > 0) ? (float)VOLUME : 0.0f;
    float step = (f > 0) ? (2.0f * 3.14159265f * f / SAMPLE_RATE) : 0.0f;
    for (int i = 0; i < 160; i++) {
      amp += (target - amp) * 0.02f;   // ふわっと 大きく・小さく（ぶつっ という音を へらす）
      phase += step;
      if (phase > 6.2831853f) phase -= 6.2831853f;
      float v = (sinf(phase) + 0.3f * sinf(phase * 3.0f)) * amp * 0.75f;
      if (bellAmp > 0.001f) {       // かねの 音: 高い 音が まざって、だんだん 小さくなる
        bellPhase += bellStep;
        float b = sinf(bellPhase) + 0.5f * sinf(bellPhase * 2.76f) + 0.25f * sinf(bellPhase * 5.4f);
        v += b * bellAmp * VOLUME * 0.5f;
        bellAmp *= 0.9993f;         // 「カーン」の 長さ（1に ちかいほど 長い）
      }
      buf[i] = (int16_t)v;
    }
    i2s.write((uint8_t *)buf, sizeof(buf));
  }
}

void audioBegin() {
  i2s.setPins(I2S_BCLK, I2S_LRC, I2S_DIN);
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("アンプ（MAX98357A）が はじめられません。配線を たしかめてください");
    return;
  }
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, NULL, 1, NULL, 0);
}

TouchPad pad1 = {14, 0, false, 0, 0, 0, false, false, false, false, 0};
TouchPad pad2 = {27, 0, false, 0, 0, 0, false, false, false, false, 0};

// ---- 0と1の絵を、画面にかく（scale は 1ドットを 何倍で出すか）----
void drawMarkS(const char *const mark[], int x, int y, int scale) {
  for (int row = 0; row < MARK_SIZE; row++) {
    const char *line = mark[row];
    for (int col = 0; col < MARK_SIZE && line[col] != '\0'; col++) {
      if (line[col] == '1') {
        display.drawBox(x + col * scale, y + row * scale, scale, scale);
      }
    }
  }
}
void drawMark(const char *const mark[], int x, int y) {
  drawMarkS(mark, x, y, MARK_SCALE);
}

// ---- じょうたい ----
const int ST_IDLE = 0;    // まっている
const int ST_PRE = 1;     // なりはじめ（しゃだんきは あがっている）
const int ST_LOWER = 2;   // しゃだんきが おりる
const int ST_PASS = 3;    // 通っている
const int ST_RAISE = 4;   // しゃだんきが あがる
int st = ST_IDLE;
unsigned long stMs = 0;

const int K_TRAIN = 0;
const int K_CACTUS = 1;
const int K_ART = 2;
int kind = K_TRAIN;
float passX = 0;               // 通るものの 左はし
unsigned long lastPassMs = 0;

int lampPhase = -1;            // 0:左が 光る  1:右が 光る
unsigned long alarmStart = 0;   // なりはじめた とき

// ---- 場面の かたち ----
const int GROUND_Y = 60;       // 地面（レール）の y
const int GATE_L = 26;         // 左の しゃだんきの 柱の x
const int GATE_R = 122;        // 右の しゃだんきの 柱の x
const int PIVOT_Y = 46;        // しゃだんきの 付け根の y（小さいほど 高い）
const int ARM_LEN = 46;        // しゃだんきの 長さ
const int ARM_UP_DEG = 60;     // あがっている ときの かくど（90 で まっすぐ 上）
const int ARM_DOWN_DEG = 2;    // おりている ときの かくど（0 で よこ）

// しゃだんき（あかと しろの しま もよう）
void drawArm(int px, int dir, float up) {
  // up: 1 = あがっている、0 = おりている
  float deg = ARM_DOWN_DEG + up * (ARM_UP_DEG - ARM_DOWN_DEG);
  float rad = deg * 3.14159265f / 180.0f;
  float dx = dir * cosf(rad);
  float dy = -sinf(rad);
  int n = ARM_LEN / 6;
  for (int k = 0; k < n; k++) {
    int x0 = px + (int)(dx * k * 6);
    int y0 = PIVOT_Y + (int)(dy * k * 6);
    int x1 = px + (int)(dx * (k + 1) * 6);
    int y1 = PIVOT_Y + (int)(dy * (k + 1) * 6);
    display.drawLine(x0, y0, x1, y1);
    if (k % 2 == 0) {                       // ふとい ところ（赤）
      display.drawLine(x0, y0 + 1, x1, y1 + 1);
      display.drawLine(x0, y0 + 2, x1, y1 + 2);
    }
  }
}

// ---- 画面の そとへ はみだしても だいじょうぶな かきかた ----
// U8g2 は、画面の 左の そと（x が マイナス）に はみだす 絵を かくと、反対がわ（右がわ）に
// へんな 線が 出ることが あります。そこで、はみだす ぶんは、自分で けずって から かきます。
void cPixel(int x, int y) {
  if (x >= 0 && x < 128 && y >= 0 && y < 64) display.drawPixel(x, y);
}
void cHLine(int x, int y, int w) {
  if (x < 0) { w += x; x = 0; }
  if (x + w > 128) w = 128 - x;
  if (w > 0 && y >= 0 && y < 64) display.drawHLine(x, y, w);
}
void cVLine(int x, int y, int h) {
  if (y < 0) { h += y; y = 0; }
  if (y + h > 64) h = 64 - y;
  if (h > 0 && x >= 0 && x < 128) display.drawVLine(x, y, h);
}
void cBox(int x, int y, int w, int h) {
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > 128) w = 128 - x;
  if (y + h > 64) h = 64 - y;
  if (w > 0 && h > 0) display.drawBox(x, y, w, h);
}
void cLine(int x0, int y0, int x1, int y1) {
  int dx = abs(x1 - x0), dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  while (true) {
    cPixel(x0, y0);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx) { err += dx; y0 += sy; }
  }
}
void cDisc(int cx, int cy, int r) {
  for (int dy = -r; dy <= r; dy++)
    for (int dx = -r; dx <= r; dx++)
      if (dx * dx + dy * dy <= r * r) cPixel(cx + dx, cy + dy);
}
void cRFrame(int x, int y, int w, int h) {   // かどの まるい わく
  cHLine(x + 2, y, w - 4);
  cHLine(x + 2, y + h - 1, w - 4);
  cVLine(x, y + 2, h - 4);
  cVLine(x + w - 1, y + 2, h - 4);
  cPixel(x + 1, y + 1); cPixel(x + w - 2, y + 1);
  cPixel(x + 1, y + h - 2); cPixel(x + w - 2, y + h - 2);
}
void cMark(const char *const mark[], int x, int y, int scale) {   // 0と1の絵（はみだしても OK）
  for (int row = 0; row < MARK_SIZE; row++) {
    const char *line = mark[row];
    for (int col = 0; col < MARK_SIZE && line[col] != '\0'; col++) {
      if (line[col] == '1') cBox(x + col * scale, y + row * scale, scale, scale);
    }
  }
}

// 電車の しゃりょう（x は 左はし。front が true なら 先頭＝右むき の はな つき）
void drawCar(int x, bool front, bool panto) {
  int w = front ? 22 : 24;
  cRFrame(x, 38, w, 17);                        // 車体
  cHLine(x + 1, 49, w - 2);                     // 色の おび
  cBox(x + 3, 41, 5, 5);                        // 窓
  cBox(x + 10, 41, 5, 5);
  if (!front) cBox(x + 17, 41, 5, 5);
  else cBox(x + 16, 41, 4, 5);
  cHLine(x + 2, 55, w - 4);                     // ゆか下
  cDisc(x + 6, 57, 2);                          // 車輪
  cDisc(x + w - 6, 57, 2);
  if (panto) {                                  // パンタグラフ
    cLine(x + 8, 38, x + 12, 34);
    cLine(x + 12, 34, x + 16, 38);
    cHLine(x + 9, 34, 6);
  }
  if (front) {                                  // はな（右がわの ななめ）
    cLine(x + w - 1, 38, x + w + 7, 47);
    cLine(x + w + 7, 47, x + w + 7, 55);
    cHLine(x + w - 1, 55, 8);
    cDisc(x + w + 4, 51, 1);                    // ライト
  }
}

// 通るものの うしろの 長さ（電車なら TRAIN_CARS 両ぶん、さぼてんきくんなども 同じ数だけ ならぶ）
int tailLen() {
  return (kind == K_TRAIN) ? 26 * TRAIN_CARS : 36 * (TRAIN_CARS - 1);
}

void drawPasser(unsigned long now) {
  int x = (int)passX;
  if (kind == K_TRAIN) {
    drawCar(x, true, false);
    for (int i = 0; i < TRAIN_CARS; i++) {
      int cx = x - 26 * (i + 1) + 2;
      drawCar(cx, false, i == 0);
      cHLine(cx + 24, 52, 3);                    // れんけつ
    }
  } else {
    // さぼてんきくん や MY_ART が、TRAIN_CARS こ、ならんで 通る（1こずつ ぴょんぴょん）
    for (int i = 0; i < TRAIN_CARS; i++) {
      int px0 = x - 36 * i;
      long t = (long)(now / 220) + i;
      int hop = (t % 2) ? -2 : 0;
      if (kind == K_CACTUS) cMark((t % 2) ? MARK_KAITEKI2 : MARK_KAITEKI, px0, 28 + hop, MARK_SCALE);
      else cMark(MY_ART, px0, 28 + hop, MARK_SCALE);
    }
  }
}

// けいほうき（左がわ）: ×の しるしと、赤い ランプ 2こ（左右 かわりばんこ）
void drawWarning(bool on) {
  display.drawLine(5, 14, 17, 24);               // ×
  display.drawLine(17, 14, 5, 24);
  display.drawRFrame(2, 26, 18, 11, 2);          // ランプの はこ
  if (on && lampPhase == 0) display.drawDisc(7, 31, 3); else display.drawCircle(7, 31, 3);
  if (on && lampPhase == 1) display.drawDisc(15, 31, 3); else display.drawCircle(15, 31, 3);
  display.drawBox(10, 37, 3, GROUND_Y - 37);     // 柱
}

void drawScene(unsigned long now) {
  display.clearBuffer();
  display.setFont(u8g2_font_b12_t_japanese2);

  // 地面と レール
  display.drawHLine(0, GROUND_Y, 128);
  display.drawHLine(0, GROUND_Y + 2, 128);
  for (int x = 2; x < 128; x += 6) display.drawVLine(x, GROUND_Y + 1, 2);

  // しゃだんきの 位置（up: 1.0=あがる 〜 0.0=おりる）
  float up = 1.0f;
  if (st == ST_PRE) up = 1.0f;
  else if (st == ST_LOWER) up = 1.0f - (float)(now - stMs) / LOWER_MS;
  else if (st == ST_PASS) up = 0.0f;
  else if (st == ST_RAISE) up = (float)(now - stMs) / RAISE_MS;
  if (up < 0) up = 0;
  if (up > 1) up = 1;

  drawWarning(st != ST_IDLE);

  // しゃだんきの 柱
  display.drawBox(GATE_L - 1, PIVOT_Y - 2, 3, GROUND_Y - PIVOT_Y + 2);
  display.drawBox(GATE_R - 1, PIVOT_Y - 2, 3, GROUND_Y - PIVOT_Y + 2);

  if (st == ST_PASS) drawPasser(now);

  drawArm(GATE_L, +1, up);
  drawArm(GATE_R, -1, up);

  if (st == ST_IDLE) {
    display.drawUTF8(48, 30, "ふみきり");
    display.drawUTF8(44, 46, "さわってね");
  }
  display.sendBuffer();
}

void startCrossing(unsigned long now, bool forceCactus) {
  st = ST_PRE;
  stMs = now;
  lampPhase = -1;
  alarmStart = now;
  int r = random(0, 100);
  if (forceCactus || r < CACTUS_PERCENT) kind = K_CACTUS;
  else if (r < CACTUS_PERCENT + ART_PERCENT) kind = K_ART;
  else kind = K_TRAIN;
  passX = (kind == K_TRAIN) ? -30.0f : -32.0f;   // 画面の 左の そとから はじめる
  lastPassMs = now;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  display.begin();
  display.setBusClock(400000);
  display.enableUTF8Print();
  display.setFont(u8g2_font_b12_t_japanese2);

  pinMode(LED_L, OUTPUT);
  pinMode(LED_R, OUTPUT);
  digitalWrite(LED_L, LOW);
  digitalWrite(LED_R, LOW);

  display.clearBuffer();
  display.drawUTF8(0, 30, "ふみきり");
  drawMarkS(MARK_KAITEKI, 96, 4, MARK_SCALE);
  display.drawUTF8(0, 62, "さわらないでね");
  display.sendBuffer();

  audioBegin();
  pad1.base = measureBase(pad1.pin);
  pad2.base = measureBase(pad2.pin);
  Serial.printf("タッチの きじゅん 1=%.0f 2=%.0f\n", pad1.base, pad2.base);
  randomSeed(esp_random() ^ micros());
}

void loop() {
  unsigned long now = millis();
  padUpdate(pad1, now);
  padUpdate(pad2, now);

  if (st == ST_IDLE) {
    if (pad1.pressedEdge) startCrossing(now, false);
    else if (pad2.pressedEdge) startCrossing(now, true);
  } else if (st == ST_PRE) {
    if (now - stMs >= PRE_MS) { st = ST_LOWER; stMs = now; }
  } else if (st == ST_LOWER) {
    if (now - stMs >= LOWER_MS) { st = ST_PASS; stMs = now; lastPassMs = now; }
  } else if (st == ST_PASS) {
    float speed = (kind == K_TRAIN) ? TRAIN_SPEED : WALK_SPEED;
    passX += speed * (now - lastPassMs) / 1000.0f;
    lastPassMs = now;
    if (passX - tailLen() > 128) { st = ST_RAISE; stMs = now; }   // うしろまで 画面の そとに 出たら おわり
  } else if (st == ST_RAISE) {
    if (now - stMs >= RAISE_MS) { st = ST_IDLE; stMs = now; }
  }

  // ---- LED と 音（なっている間だけ）----
  if (st == ST_IDLE) {
    digitalWrite(LED_L, LOW);
    digitalWrite(LED_R, LOW);
    lampPhase = -1;
  } else {
    int phase = ((now - alarmStart) / LAMP_MS) % 2;   // かわりばんこ
    if (phase != lampPhase) {      // 光る ほうが かわった しゅんかんに「カン」と たたく
      lampPhase = phase;
      digitalWrite(LED_L, phase == 0 ? HIGH : LOW);
      digitalWrite(LED_R, phase == 1 ? HIGH : LOW);
      g_bellHz = (phase == 0) ? BELL_HZ_A : BELL_HZ_B;   // かねを たたく（カーン）
    }
  }

  drawScene(now);
}
