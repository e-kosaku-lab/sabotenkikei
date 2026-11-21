// さぼてんきくんの はんのうゲーム（ESP32 + 0.96インチOLED）
//
// 「さぼてんきけい」の、追加パックの プログラムです。
// 画面の さぼてんきくんが「！」と なったら、すぐに ワイヤーを さわります。はやさを きそいます。
// 部品は、ESP32 と OLED と、予備のワイヤー2本だけ。温度・湿度のセンサーは、いりません。
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
//   ESP32 GPIO14（D14）→ ワイヤー1（1P）
//   ESP32 GPIO27（D27）→ ワイヤー2（2P）  ※ 1人で あそぶ ときは、なくても だいじょうぶ
//   ワイヤーの、もう一方の先（金属の部分）を、さわります。
//
// つかいかた:
//   ① ちょん、とさわると スタート。「まってね…」と 出ます。
//   ② 2〜5びょうの あと、さぼてんきくんが「！」になって、画面が 白くなります（「いま！」）。
//   ③ そのしゅんかんに、ワイヤーを ちょん！ はやさ（びょう）が 出ます。
//   ④ 「いま！」の まえに さわると「フライング」。
//   ⑤ いちばん はやい きろくは、ボードの 中に のこります（電源を切っても のこります）。
//   ※ 電源を入れる時は、ワイヤーの先に、さわらないでください。

#include <Wire.h>
#include <U8g2lib.h>
#include <Preferences.h>

// =====================================================================
//  かえてみよう！（ここから下を書きかえると、ゲームがかわります）
//  書きかえたら、もう一度「書き込み」ボタン（→）をおしてね。
// =====================================================================

// ---- ① ゲームを かえてみよう ----
const unsigned long WAIT_MIN_MS = 2000;   // 「いま！」までの、いちばん みじかい 時間（ミリ秒）
const unsigned long WAIT_MAX_MS = 5000;   // 「いま！」までの、いちばん ながい 時間（ミリ秒）
const unsigned long RATE_GREAT = 250;     // これより はやいと「すごい！」（ミリ秒）
const unsigned long RATE_GOOD = 400;      // これより はやいと「はやい！」
const unsigned long RATE_NORMAL = 600;    // これより はやいと「ふつう」。それより おそいと「ねむそう」

// ---- ② 言葉を かえてみよう ----
const char *WORD_GREAT = "すごい!";
const char *WORD_GOOD = "はやい!";
const char *WORD_NORMAL = "ふつう";
const char *WORD_SLOW = "ねむそう";

// ---- ③ タッチの ぐあいを かえてみよう ----
// 0.3〜0.9 の間で。反応しすぎる時は 小さく、反応しない時は 大きく。
const float TOUCH_RATIO = 0.6;
const int TOUCH_CONFIRM = 3;               // 何回つづけて 同じなら さわったと みなすか
const unsigned long LONG_MS = 1200;        // ながく さわる 長さ（ミリ秒）
const unsigned long ANIME_MS = 500;        // さぼてんきくんの 絵の きりかえの間かく（ミリ秒）。50より小さくしないでね

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
Preferences prefs;

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

const int S_IDLE = 0;    // はじめの画面
const int S_WAIT = 1;    // まってね…
const int S_GO = 2;      // いま！
const int S_RESULT = 3;  // けっか
int state = S_IDLE;

unsigned long stateMs = 0;    // この状態に なった とき
unsigned long waitMs = 0;     // 「いま！」までの 時間
unsigned long goMs = 0;       // 「いま！」を 出した とき
unsigned long bestMs = 99999; // いちばん はやい きろく
bool fly = false;             // フライング だったか
int who = 0;                  // さわった人（1 か 2）
unsigned long reactMs = 0;    // はんのう 時間
bool newBest = false;
bool needDraw = true;

void drawMarkFrame(const char *const a[], const char *const b[], int x, int y, int scale, unsigned long now) {
  int frame = (now / ANIME_MS) % 2;
  drawMarkS(frame ? b : a, x, y, scale);
}

void formatSec(char *out, size_t n, unsigned long ms) {
  snprintf(out, n, "%.2fびょう", ms / 1000.0f);
}

void drawScreen(unsigned long now) {
  char line[40];
  display.clearBuffer();
  display.setFont(u8g2_font_b12_t_japanese2);

  if (state == S_IDLE) {
    drawMarkFrame(MARK_KAITEKI, MARK_KAITEKI2, 96, 4, MARK_SCALE, now);
    display.drawUTF8(0, 13, "はんのうゲーム");
    display.drawUTF8(0, 29, "ちょんと");
    display.drawUTF8(0, 45, "スタート!");
    if (bestMs < 99999) {
      char t[24];
      formatSec(t, sizeof(t), bestMs);
      snprintf(line, sizeof(line), "ベスト %s", t);
      display.drawUTF8(0, 62, line);
    } else {
      display.drawUTF8(0, 62, "ベスト ---");
    }
  } else if (state == S_WAIT) {
    drawMarkS(MARK_KAITEKI, 48, 4, 3);
    display.drawUTF8(0, 62, "まってね…");
  } else if (state == S_GO) {
    // 画面が 白くなる（さぼてんきくんは びっくり）
    display.drawBox(0, 0, 128, 64);
    display.setDrawColor(0);
    drawMarkS(MARK_ITAI, 8, 8, 3);
    display.setFont(u8g2_font_logisoso28_tn);
    display.setDrawColor(0);
    display.drawStr(76, 40, "!");
    display.setFont(u8g2_font_b12_t_japanese2);
    display.drawUTF8(60, 62, "いま!");
    display.setDrawColor(1);
  } else {
    // けっか
    const char *const *m = fly ? MARK_ITAI : MARK_KAITEKI;
    const char *const *m2 = fly ? MARK_ITAI2 : MARK_KAITEKI2;
    const char *word = "";
    if (!fly) {
      if (reactMs < RATE_GREAT) { m = MARK_TERE; m2 = MARK_TERE2; word = WORD_GREAT; }
      else if (reactMs < RATE_GOOD) word = WORD_GOOD;
      else if (reactMs < RATE_NORMAL) word = WORD_NORMAL;
      else { m = MARK_KARAKARA; m2 = MARK_KARAKARA2; word = WORD_SLOW; }
    }
    drawMarkFrame(m, m2, 96, 4, MARK_SCALE, now);
    if (fly) {
      snprintf(line, sizeof(line), "%dP フライング!", who);
      display.drawUTF8(0, 13, line);
      display.drawUTF8(0, 29, "はやすぎ!");
    } else {
      char t[24];
      formatSec(t, sizeof(t), reactMs);
      snprintf(line, sizeof(line), "%dP", who);
      display.drawUTF8(0, 13, line);
      display.drawUTF8(0, 29, t);
      display.drawUTF8(0, 45, word);
    }
    if (newBest) {
      display.drawUTF8(0, 62, "しんきろく!");
    } else if (bestMs < 99999) {
      char t[24];
      formatSec(t, sizeof(t), bestMs);
      snprintf(line, sizeof(line), "ベスト %s", t);
      display.drawUTF8(0, 62, line);
    }
  }
  display.sendBuffer();
}

void startWait(unsigned long now) {
  state = S_WAIT;
  stateMs = now;
  waitMs = WAIT_MIN_MS + random(0, (long)(WAIT_MAX_MS - WAIT_MIN_MS + 1));
  needDraw = true;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  display.begin();
  display.setBusClock(400000);
  display.enableUTF8Print();
  display.setFont(u8g2_font_b12_t_japanese2);

  display.clearBuffer();
  display.drawUTF8(0, 30, "はんのうゲーム");
  drawMarkS(MARK_KAITEKI, 96, 4, MARK_SCALE);
  display.drawUTF8(0, 62, "さわらないでね");
  display.sendBuffer();

  pad1.base = measureBase(pad1.pin);
  pad2.base = measureBase(pad2.pin);
  Serial.printf("タッチの きじゅん 1=%.0f 2=%.0f\n", pad1.base, pad2.base);

  prefs.begin("hannou", false);
  bestMs = prefs.getULong("best", 99999);
  randomSeed(esp_random() ^ micros());
}

void loop() {
  unsigned long now = millis();
  padUpdate(pad1, now);
  padUpdate(pad2, now);
  bool p1 = pad1.pressedEdge;
  bool p2 = pad2.pressedEdge;

  if (state == S_IDLE) {
    if (p1 || p2) startWait(now);
  } else if (state == S_WAIT) {
    if (p1 || p2) {
      // 「いま！」の まえに さわった → フライング
      fly = true;
      who = p1 ? 1 : 2;
      newBest = false;
      state = S_RESULT;
      stateMs = now;
      needDraw = true;
    } else if (now - stateMs >= waitMs) {
      state = S_GO;
      drawScreen(now);   // 先に 画面を 出してから、時間を はかりはじめる
      goMs = millis();
      stateMs = goMs;
    }
  } else if (state == S_GO) {
    if (p1 || p2) {
      who = p1 ? 1 : 2;
      unsigned long t = (who == 1) ? pad1.downMs : pad2.downMs;
      reactMs = (t > goMs) ? (t - goMs) : 0;
      fly = false;
      newBest = false;
      if (reactMs > 0 && reactMs < bestMs) {
        bestMs = reactMs;
        prefs.putULong("best", bestMs);
        newBest = true;
      }
      Serial.printf("%dP %lu ミリびょう\n", who, reactMs);
      state = S_RESULT;
      stateMs = now;
      needDraw = true;
    } else if (now - stateMs >= 5000) {
      // 5びょう たっても さわらない → もどる
      state = S_IDLE;
      needDraw = true;
    }
  } else {  // S_RESULT
    if ((p1 || p2) && now - stateMs >= 800) startWait(now);   // すぐ もういちど
    if (pad1.longEdge) { state = S_IDLE; needDraw = true; }    // ながく さわると はじめの画面へ
  }

  if (state != S_WAIT && state != S_GO) needDraw = true;       // 絵を うごかすため
  if (state == S_WAIT) needDraw = true;
  if (needDraw && state != S_GO) {
    drawScreen(now);
    needDraw = false;
  }
  delay(8);
}
