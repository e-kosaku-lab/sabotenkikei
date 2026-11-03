// Lチカ（出荷前チェック用 / 説明書の最初の練習用）
// ボード上のLEDを1秒ごとに点滅させます。Wi-Fi・Bluetoothは使いません。
// NodeMCU-32S の青いLEDは GPIO2 につながっています。
// ※商品によってLEDが無い・ピンが違うことがあります。出荷前に実機で確認してください。

const int LED_PIN = 2;

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
}
