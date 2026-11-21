// さぼてんきくんの 電子サイコロ・おみくじ（ESP32 + 0.96インチOLED）
//
// 「さぼてんきけい」の、追加パックの プログラムです。さぼてんきくんが、サイコロを 見まもります。
// サイコロは 1こ でも 2こ でも ふれます。
// 部品は、ESP32 と OLED と、予備のワイヤー1本だけ。温度・湿度のセンサーは、いりません。
//
// このプログラムは Wi-Fi / Bluetooth を一切使いません（WiFi.h などを読み込んでいません）。
//
// 必要なライブラリ: U8g2 （olikraus）
// ボード設定: ツール → ボード → esp32 → 「ESP32 Dev Module」
//
// 配線:
//   ESP32 3V3     → OLED VDD
//   ESP32 GND     → OLED GND
//   ESP32 GPIO21  → OLED SDA
//   ESP32 GPIO22  → OLED SCK
//   ESP32 GPIO14（D14）→ ワイヤー1（さわる場所）。もう一方の先を、さわります。
//
// つかいかた:
//   ちょん、とさわる        … サイコロを ふる（さぼてんきくんが 見まもる）
//   ながく（1.2びょう）さわる … 「サイコロ1こ」→「サイコロ2こ」→「おみくじ」の じゅんに きりかえる
//   サイコロ1こ: 6が でると さぼてんきくんは「えへへ」、1が でると「いたい！」
//   サイコロ2こ: ぞろめ（同じ目）で「えへへ」、1と1で「いたい！」。ごうけいも 出ます
//   ※ 電源を入れる時は、ワイヤーの先に、さわらないでください。

#include <Wire.h>
#include <U8g2lib.h>

// =====================================================================
//  かえてみよう！（ここから下を書きかえると、動きがかわります）
//  書きかえたら、もう一度「書き込み」ボタン（→）をおしてね。
// =====================================================================

// ---- ① サイコロを かえてみよう ----
const int DICE_MAX = 6;                    // サイコロの 面の数。6を 8 や 12 にしてみよう（7からは 数字で出ます）
const unsigned long ROLL_MS = 1500;        // ころがっている 長さ（ミリ秒）
const unsigned long ANIME_MS = 500;        // さぼてんきくんの 絵の きりかえの間かく（ミリ秒）。50より小さくしないでね

// ---- ② おみくじを かえてみよう ----
// ひらがな・カタカナが おすすめ。むずかしい漢字は、画面に出ないことがあります。
// 先頭が いちばん いい運、さいごが いちばん わるい運 として、さぼてんきくんが はんのうします。
const char *OMIKUJI[] = {"だいきち", "ちゅうきち", "しょうきち", "すえきち", "きょう"};
const int OMIKUJI_N = 5;                   // ↑の 言葉の かず（かえたら、おなじ数に）

// ---- ③ タッチの ぐあいを かえてみよう ----
// 0.3〜0.9 の間で。反応しすぎる時は 小さく、反応しない時は 大きく。
const float TOUCH_RATIO = 0.6;
const int TOUCH_CONFIRM = 3;               // 何回つづけて 同じなら さわったと みなすか
const unsigned long LONG_MS = 1200;        // ながく さわる 長さ（ミリ秒）

// ---- ④ 0と1で 絵をかえてみよう ----
// 1 が「光るドット」、0 が「光らないドット」です。1行に16こ、16行あります。
// 名前のさいごに 2 がついているのが、絵2 です。（交互に出して、動いて見えます）

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

TouchPad pad1 = {14, 0, false, 0, 0, 0, false, false, false, false, 0};

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

const int MODE_DICE1 = 0;    // サイコロ 1こ
const int MODE_DICE2 = 1;    // サイコロ 2こ
const int MODE_OMIKUJI = 2;  // おみくじ
const int MODE_N = 3;
int mode = MODE_DICE1;

bool rolling = false;
unsigned long rollStart = 0;
unsigned long nextChange = 0;
int shown[2] = {1, 1};    // いま 画面に出している 目（または おみくじの ばんごう）
int result[2] = {0, 0};   // さいごに きまった 目
bool hasResult = false;

int pickValue() {
  if (mode != MODE_OMIKUJI) return random(1, DICE_MAX + 1);
  return random(0, OMIKUJI_N);
}

void drawPip(int cx, int cy, int r) {
  display.drawDisc(cx, cy, r);
}

// サイコロの 面（左上が x,y。大きさ s）
void drawDiceFace(int v, int x, int y, int s) {
  display.drawRFrame(x, y, s, s, 6);
  if (v > 6) {
    char t[8];
    snprintf(t, sizeof(t), "%d", v);
    if (s >= 48) display.setFont(u8g2_font_logisoso28_tn);
    else display.setFont(u8g2_font_logisoso20_tn);
    int w = display.getStrWidth(t);
    display.drawStr(x + (s - w) / 2, y + s / 2 + (s >= 48 ? 14 : 10), t);
    display.setFont(u8g2_font_b12_t_japanese2);
    return;
  }
  int off = s / 4;          // はしから 点までの 距離
  int r = s / 12;           // 点の 大きさ
  if (r < 2) r = 2;
  int a = x + off, b = x + s / 2, c = x + s - off;
  int r1 = y + off, r2 = y + s / 2, r3 = y + s - off;
  switch (v) {
    case 1: drawPip(b, r2, r); break;
    case 2: drawPip(a, r1, r); drawPip(c, r3, r); break;
    case 3: drawPip(a, r1, r); drawPip(b, r2, r); drawPip(c, r3, r); break;
    case 4: drawPip(a, r1, r); drawPip(c, r1, r); drawPip(a, r3, r); drawPip(c, r3, r); break;
    case 5: drawPip(a, r1, r); drawPip(c, r1, r); drawPip(b, r2, r); drawPip(a, r3, r); drawPip(c, r3, r); break;
    default:
      drawPip(a, r1, r); drawPip(a, r2, r); drawPip(a, r3, r);
      drawPip(c, r1, r); drawPip(c, r2, r); drawPip(c, r3, r);
      break;
  }
}

// さぼてんきくんの 顔（ころがっている間は ぴょんぴょん）
void drawCactus(unsigned long now) {
  const char *const *m;
  int hop = 0;
  int frame = (now / ANIME_MS) % 2;
  if (rolling) {
    frame = (now / 120) % 2;
    hop = frame ? -3 : 0;
    m = frame ? MARK_KAITEKI2 : MARK_KAITEKI;
  } else if (hasResult) {
    bool great, bad;
    if (mode == MODE_DICE1) {
      great = (result[0] == DICE_MAX);
      bad = (result[0] == 1 && DICE_MAX > 1);
    } else if (mode == MODE_DICE2) {
      bool same = (result[0] == result[1]);          // ぞろめ
      bad = same && result[0] == 1 && DICE_MAX > 1;  // 1と1
      great = same && !bad;
    } else {
      great = (result[0] == 0);
      bad = (result[0] == OMIKUJI_N - 1);
    }
    if (great) m = frame ? MARK_TERE2 : MARK_TERE;
    else if (bad) m = frame ? MARK_ITAI2 : MARK_ITAI;
    else m = frame ? MARK_KAITEKI2 : MARK_KAITEKI;
  } else {
    m = frame ? MARK_KAITEKI2 : MARK_KAITEKI;
  }
  drawMarkS(m, 84, 4 + hop, MARK_SCALE);
}

void drawScreen(unsigned long now) {
  char line[32];
  display.clearBuffer();
  display.setFont(u8g2_font_b12_t_japanese2);
  bool show = rolling || hasResult;
  if (mode == MODE_DICE1) {
    drawDiceFace(show ? shown[0] : 1, 6, 8, 48);
    if (rolling) {
      display.drawUTF8(56, 62, "ごろごろ…");
    } else if (hasResult) {
      snprintf(line, sizeof(line), "%d が でた!", result[0]);
      display.drawUTF8(56, 62, line);
    } else {
      display.drawUTF8(56, 62, "さわって!");
    }
  } else if (mode == MODE_DICE2) {
    drawDiceFace(show ? shown[0] : 1, 4, 8, 36);
    drawDiceFace(show ? shown[1] : 1, 44, 8, 36);
    if (rolling) {
      display.drawUTF8(4, 62, "ごろごろ…");
    } else if (hasResult) {
      int sum = result[0] + result[1];
      if (result[0] == result[1]) snprintf(line, sizeof(line), "ぞろめ! ごうけい %d", sum);
      else snprintf(line, sizeof(line), "ごうけい %d", sum);
      display.drawUTF8(4, 62, line);
    } else {
      display.drawUTF8(4, 62, "さわって!");
    }
  } else {
    display.drawUTF8(4, 20, "きょうは…");
    if (rolling) display.drawUTF8(4, 44, OMIKUJI[shown[0]]);
    else if (hasResult) display.drawUTF8(4, 44, OMIKUJI[result[0]]);
    else display.drawUTF8(4, 44, "？");
    display.drawUTF8(56, 62, rolling ? "ごろごろ…" : "おみくじ");
  }
  drawCactus(now);
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
  display.drawUTF8(0, 30, "さいころ");
  drawMarkS(MARK_KAITEKI, 84, 4, MARK_SCALE);
  display.drawUTF8(0, 62, "さわらないでね");
  display.sendBuffer();

  pad1.base = measureBase(pad1.pin);
  Serial.printf("タッチの きじゅん=%.0f\n", pad1.base);
  randomSeed(esp_random() ^ micros());
}

void loop() {
  unsigned long now = millis();
  padUpdate(pad1, now);

  if (pad1.longEdge) {
    mode = (mode + 1) % MODE_N;
    rolling = false;
    hasResult = false;
  } else if (pad1.releasedEdge && pad1.heldMs < LONG_MS && !rolling) {
    // ちょん（はなした とき）で ふる
    rolling = true;
    hasResult = false;
    rollStart = now;
    nextChange = now;
  }

  if (rolling) {
    if (now >= nextChange) {
      shown[0] = pickValue();
      shown[1] = pickValue();
      float progress = (float)(now - rollStart) / ROLL_MS;
      if (progress > 1) progress = 1;
      nextChange = now + 60 + (unsigned long)(progress * 220);  // だんだん ゆっくりに
    }
    if (now - rollStart >= ROLL_MS) {
      rolling = false;
      hasResult = true;
      result[0] = pickValue();
      result[1] = pickValue();
      shown[0] = result[0];
      shown[1] = result[1];
      Serial.printf("けっか=%d, %d\n", result[0], result[1]);
    }
  }

  drawScreen(now);
  delay(30);
}
