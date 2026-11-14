// Lチカ（動作チェック用）：ボードの青いLEDを、1秒ごとに、ついたり消えたりさせます
const int LED_PIN = 2;  // 青いLEDは、2番のピンにつながっています

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);  // LEDをつける
  delay(1000);                  // 1000 = 1秒まつ
  digitalWrite(LED_PIN, LOW);   // LEDを消す
  delay(1000);                  // 1000 = 1秒まつ
}
