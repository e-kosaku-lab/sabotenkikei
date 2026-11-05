# おへやのきろくモニター

ESP32 と 温湿度・気圧センサー（AHT20 + BMP280）、0.96インチOLEDで、
**温度・湿度・気圧**を日本語で表示する、はじめての電子工作のサンプルプログラムです。

- 説明書（4つのステップ）: [docs/manual.md](docs/manual.md)
- おうちの人へ（パソコンの準備・AIの使いかた・困ったとき）: [docs/for-parents.md](docs/for-parents.md)
- Lチカ（動作チェック用）: [sketches/blink/blink.ino](sketches/blink/blink.ino)
- おへやのきろくモニター: [sketches/oheya-monitor/oheya-monitor.ino](sketches/oheya-monitor/oheya-monitor.ino)

## 使うライブラリ（Arduino IDE のライブラリマネージャから）
- U8g2（olikraus）
- Adafruit BMP280 Library（依存ライブラリもいっしょに入れる）

## 注意
- このプログラムは Wi-Fi・Bluetooth を使いません。
- 技適マークのない無線機器で、日本国内でWi-Fi・Bluetoothを使うと電波法違反になるおそれがあります。無線を使うときは、ご自身で法令を確認してください。
- 学習用のサンプルです。動作や安全について、保証はありません。

## ライセンス
MIT License（[LICENSE](LICENSE)）
