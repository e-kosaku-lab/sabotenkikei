// さぼてんきくんの さわってドレミ（ESP32 + 0.96インチOLED + アンプ・スピーカー）
//
// 「さぼてんきけい」の、追加パックの プログラムです。
// ワイヤー（や、アルミホイル、くだものなど）を さわると、「ドレミ」の音が 鳴ります。
// 音に あわせて、さぼてんきくんが おどります。
//
// このプログラムは Wi-Fi / Bluetooth を一切使いません（WiFi.h などを読み込んでいません）。
//
// 必要なライブラリ: U8g2 （olikraus）
// ボード設定: ツール → ボード → esp32 → 「ESP32 Dev Module」
//
// 必要な部品（キットのほかに）:
//   ・アンプ MAX98357A モジュール（I2S）と、スピーカー
//   ・ワイヤー 6本（さわる場所 用。ジャンパーワイヤーの オス側）
//
// 配線:
//   ESP32 3V3     → OLED VDD
//   ESP32 GND     → OLED GND
//   ESP32 GPIO21  → OLED SDA
//   ESP32 GPIO22  → OLED SCK
//   ESP32 GPIO26  → アンプ BCLK
//   ESP32 GPIO25  → アンプ LRC
//   ESP32 GPIO18  → アンプ DIN
//   ESP32 5V（VIN）→ アンプ VIN（アンプは、5Vでも 3.3Vでも 動きます）
//   ESP32 GND     → アンプ GND
//   アンプの 出力 → スピーカー（＋と −）
//   さわる場所（ワイヤーの、もう一方の先は、どこにも つながない）:
//     GPIO4  → ド     GPIO13 → レ     GPIO14 → ミ
//     GPIO27 → ファ   GPIO32 → ソ     GPIO33 → ラ
//
// つかいかた:
//   ワイヤーの先を さわると、その音が 鳴ります。さぼてんきくんは、音の 場所へ 行って おどります。
//   2本いっしょに さわった ときは、あとから さわった ほうの 音が 鳴ります。
//   「ド」の ワイヤーを 2びょう さわりつづけると、「お手本モード」に なります。
//     お手本モードでは、つぎに さわる 鍵盤の 上に ▼ が 出ます。じゅんばんに さわると、曲が 弾けます。
//     曲は、「めりーさんの ひつじ」「きらきらぼし」「ちょうちょう」の 3つ。
//     もういちど 2びょう さわりつづけると、つぎの 曲 → ... → じゆうに ひく、と きりかわります。
//   ※ 電源を入れる時は、ワイヤーの先に、さわらないでください。

#include <Wire.h>
#include <U8g2lib.h>
#include <ESP_I2S.h>

// =====================================================================
//  かえてみよう！（ここから下を書きかえると、音や 動きがかわります）
//  書きかえたら、もう一度「書き込み」ボタン（→）をおしてね。
// =====================================================================

// ---- ① 音を かえてみよう ----
// 音の 高さ（Hz）。数字が 大きいほど 高い音です。ド・レ・ミ・ファ・ソ・ラ の じゅんばん。
const int NOTE_HZ[6] = {523, 587, 659, 698, 784, 880};
// 画面の 鍵盤に 出る 文字（ひらがな・カタカナが おすすめ）
const char *NOTE_NAME[6] = {"ド", "レ", "ミ", "ファ", "ソ", "ラ"};
const int VOLUME = 4000;   // 音の 大きさ（0〜20000。大きすぎると ひずみます）

// ---- ② お手本の 曲を かえてみよう ----
// 音の ばんごう: ド=0  レ=1  ミ=2  ファ=3  ソ=4  ラ=5
// 同じ 音が つづく ところは、1回ずつ、はなして、さわりなおします。
// 曲の 名前は、画面の 左上に 出ます（ぜんぶで 6もじ まで）。
const uint8_t SONG_MARY[] = {              // めりーさんの ひつじ
  2, 1, 0, 1, 2, 2, 2,   1, 1, 1,   2, 4, 4,
  2, 1, 0, 1, 2, 2, 2, 2,   1, 1, 2, 1, 0};
const uint8_t SONG_KIRAKIRA[] = {          // きらきらぼし
  0, 0, 4, 4, 5, 5, 4,   3, 3, 2, 2, 1, 1, 0,
  4, 4, 3, 3, 2, 2, 1,   4, 4, 3, 3, 2, 2, 1,
  0, 0, 4, 4, 5, 5, 4,   3, 3, 2, 2, 1, 1, 0};
const uint8_t SONG_CHOUCHOU[] = {          // ちょうちょう
  4, 2, 2,   3, 1, 1,   0, 1, 2, 3, 4, 4, 4,
  4, 2, 2, 2,   3, 1, 1, 1,   0, 2, 4, 4, 2,
  1, 1, 1, 1, 1, 2, 3,   2, 2, 2, 2, 2, 3, 4,
  4, 2, 2,   3, 1, 1,   0, 2, 4, 4, 0};
struct Song {
  const char *name;
  const uint8_t *notes;
  int n;
};
const Song SONGS[] = {
  {"めりーさん", SONG_MARY, (int)sizeof(SONG_MARY)},
  {"きらきらぼし", SONG_KIRAKIRA, (int)sizeof(SONG_KIRAKIRA)},
  {"ちょうちょう", SONG_CHOUCHOU, (int)sizeof(SONG_CHOUCHOU)},
};
const int SONG_N = 3;                      // 曲の かず（かえたら、おなじ数に）
const unsigned long MODE_HOLD_MS = 2000;   // 「ド」を これだけ さわりつづけると、お手本モードが きりかわる（ミリ秒）

// ---- ③ さぼてんきくんの うごきを かえてみよう ----
const unsigned long DANCE_MS = 120;   // おどりの 絵の きりかえの間かく（ミリ秒）。小さいほど はやい
const unsigned long ANIME_MS = 500;   // 音が ない ときの 絵の きりかえの間かく（ミリ秒）

// ---- ④ タッチの ぐあいを かえてみよう ----
// 0.3〜0.9 の間で。反応しすぎる時は 小さく、反応しない時は 大きく。
const float TOUCH_RATIO = 0.6;
const int TOUCH_CONFIRM = 2;               // 何回つづけて 同じなら さわったと みなすか（小さいほど すばやい）
const unsigned long LONG_MS = 1200;

// ---- ⑤ 0と1で 絵をかえてみよう ----
// 1 が「光るドット」、0 が「光らないドット」です。1行に16こ、16行あります。
// 名前のさいごに 2 がついているのが、絵2 です。（交互に出して、おどって見えます）

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

const int NPADS = 6;
TouchPad pads[NPADS] = {
  {4, 0, false, 0, 0, 0, false, false, false, false, 0},
  {13, 0, false, 0, 0, 0, false, false, false, false, 0},
  {14, 0, false, 0, 0, 0, false, false, false, false, 0},
  {27, 0, false, 0, 0, 0, false, false, false, false, 0},
  {32, 0, false, 0, 0, 0, false, false, false, false, 0},
  {33, 0, false, 0, 0, 0, false, false, false, false, 0},
};
unsigned long lastPress[NPADS] = {0, 0, 0, 0, 0, 0};

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

int current = -1;          // いま 鳴っている 音（-1 は しずか）
float cactusX = 48;        // さぼてんきくんの よこいち

const int KEY_W = 20;
const int KEY_X0 = 1;      // 6こ × 21 ドット = 126
const int KEY_Y = 48;

// ---- お手本モード ----
int songMode = -1;         // -1: じゆうに ひく　0〜: お手本の 曲ばんごう
int songPos = 0;           // 何ばんめの 音か
bool songDone = false;     // 弾きおわった
unsigned long doneMs = 0;
unsigned long missUntil = 0;   // まちがえた ときの 顔の 長さ
bool modeFired = false;

void drawScene(unsigned long now) {
  display.clearBuffer();
  display.setFont(u8g2_font_b12_t_japanese2);
  bool teach = (songMode >= 0);

  // さぼてんきくん
  const char *const *m;
  int hop = 0;
  bool dance = (current >= 0) || songDone;
  int f = (now / DANCE_MS) % 2;
  if (teach && now < missUntil) {
    m = f ? MARK_ITAI2 : MARK_ITAI;
  } else if (dance) {
    hop = f ? -5 : 0;
    m = f ? MARK_TERE2 : MARK_KAITEKI2;
    if (current >= 3 || songDone) m = f ? MARK_TERE : MARK_TERE2;
  } else {
    m = ((now / ANIME_MS) % 2) ? MARK_KAITEKI2 : MARK_KAITEKI;
  }
  int cy = teach ? 6 : 11;
  drawMarkS(m, (int)cactusX, cy + (hop < 0 ? hop : 0), MARK_SCALE);

  // お手本の 表示
  if (teach) {
    char t[24];
    display.drawUTF8(0, 13, SONGS[songMode].name);
    if (songDone) snprintf(t, sizeof(t), "できた!");
    else snprintf(t, sizeof(t), "%d/%d", songPos + 1, SONGS[songMode].n);
    display.drawUTF8(0, 29, t);
    if (!songDone && (now / 300) % 2 == 0) {
      int tn = SONGS[songMode].notes[songPos];
      int cx = KEY_X0 + tn * (KEY_W + 1) + KEY_W / 2;
      display.drawTriangle(cx - 4, 40, cx + 4, 40, cx, 46);   // つぎは ここ！
    }
  }

  // 鍵盤
  display.setFont(u8g2_font_b10_t_japanese1);
  for (int i = 0; i < NPADS; i++) {
    int x = KEY_X0 + i * (KEY_W + 1);
    bool on = (i == current);
    if (on) display.drawBox(x, KEY_Y, KEY_W, 16);
    else display.drawFrame(x, KEY_Y, KEY_W, 16);
    display.setDrawColor(on ? 0 : 1);
    int w = display.getUTF8Width(NOTE_NAME[i]);
    display.drawUTF8(x + (KEY_W - w) / 2, KEY_Y + 12, NOTE_NAME[i]);
    display.setDrawColor(1);
  }
  display.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  display.begin();
  display.setBusClock(400000);
  display.enableUTF8Print();
  display.setFont(u8g2_font_b12_t_japanese2);

  display.clearBuffer();
  display.drawUTF8(0, 30, "さわってドレミ");
  drawMarkS(MARK_KAITEKI, 48, 4, 1);
  display.drawUTF8(0, 62, "さわらないでね");
  display.sendBuffer();

  audioBegin();
  for (int i = 0; i < NPADS; i++) pads[i].base = measureBase(pads[i].pin);
  Serial.println("タッチの きじゅん:");
  for (int i = 0; i < NPADS; i++) Serial.printf("  GPIO%d = %.0f\n", pads[i].pin, pads[i].base);
}

void loop() {
  unsigned long now = millis();
  for (int i = 0; i < NPADS; i++) {
    padUpdate(pads[i], now);
    if (pads[i].pressedEdge) lastPress[i] = now;
  }

  // 「ド」を 2びょう さわりつづけると、お手本モードを きりかえる
  if (pads[0].pressedEdge) modeFired = false;
  if (pads[0].down && !modeFired && now - pads[0].downMs >= MODE_HOLD_MS) {
    modeFired = true;
    songMode++;
    if (songMode >= SONG_N) songMode = -1;
    songPos = 0;
    songDone = false;
    Serial.printf("お手本モード = %d\n", songMode);
  }

  // さわっている 中で、いちばん あとから さわった ものの 音を 鳴らす
  // （モードを きりかえている 間の「ド」は、しずかに する）
  int best = -1;
  for (int i = 0; i < NPADS; i++) {
    if (!pads[i].down) continue;
    if (i == 0 && modeFired) continue;
    if (best < 0 || lastPress[i] > lastPress[best]) best = i;
  }
  current = best;
  g_freq = (best >= 0) ? NOTE_HZ[best] : 0;

  // お手本: つぎの 音を さわったら すすむ
  if (songMode >= 0) {
    if (songDone) {
      if (now - doneMs > 3000) { songDone = false; songPos = 0; }   // 3びょう あと、さいしょから
    } else {
      for (int i = 0; i < NPADS; i++) {
        if (!pads[i].pressedEdge || (i == 0 && modeFired)) continue;
        if (i == SONGS[songMode].notes[songPos]) {
          songPos++;
          if (songPos >= SONGS[songMode].n) { songDone = true; doneMs = now; }
        } else {
          missUntil = now + 400;      // ちがう 音
        }
      }
    }
  }

  // さぼてんきくんは、音の 場所へ（お手本モードでは 右がわに いる）
  float target = cactusX;
  if (songMode >= 0) {
    target = 92;
  } else if (current >= 0) {
    target = KEY_X0 + current * (KEY_W + 1) + KEY_W / 2 - MARK_SIZE * MARK_SCALE / 2;
  }
  if (target < 0) target = 0;
  if (target > 128 - MARK_SIZE * MARK_SCALE) target = 128 - MARK_SIZE * MARK_SCALE;
  cactusX += (target - cactusX) * 0.35f;

  drawScene(now);
}
