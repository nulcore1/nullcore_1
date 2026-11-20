# Промпт для Claude Code — собрать прошивку nullcore

Скопируй весь текст ниже и вставь в Claude Code (в терминале, в пустой папке).

---

Собери мне кастомную прошивку "nullcore" на основе Bruce для платы ESP32-S3 N16R8
с экраном ST7789 2.0" и физическими кнопками (без сенсора). Делай по шагам:

## 1. Склонируй Bruce
```
git clone https://github.com/pr3y/Bruce.git
cd Bruce
```

## 2. Создай папку boards/nullcore/ с двумя файлами

**boards/nullcore/nullcore.ini** — конфиг платы. Содержимое:
(возьми из файла nullcore.ini, который я приложу, или создай с таким пинаутом:
Display ST7789 SPI: SCLK=12 MOSI=11 MISO=13 CS=10 DC=9 RST=14 BL=21;
CC1101: SCK=12 MISO=13 MOSI=11 CS=15 GDO0=16;
nRF24: SCK=12 MISO=13 MOSI=11 CS=17 CE=18;
PN532 I2C: SDA=41 SCL=42; IR: TX=40 RX=39;
Buttons: UP=4 DOWN=5 LEFT=6 RIGHT=7 OK=8; env name = nullcore;
board=esp32s3, memory qio_opi, partitions custom_16Mb.csv, USB_as_HID=1 для BadUSB)

**boards/nullcore/interface.cpp** — обработка 5 кнопок вместо сенсора и настройка
пинов модулей (bruceConfigPins для CC1101_bus, NRF24_bus, i2c_bus, irTx/irRx;
InputHandler читает кнопки с INPUT_PULLUP и ставит PrevPress/NextPress/SelPress/EscPress).

Я приложу оба файла — просто положи их в boards/nullcore/.

## 3. Поменяй загрузочную заставку на "nullcore"
В файле `src/main.cpp`, в функции `boot_screen()`, найди:
```cpp
    tft.drawCentreString("Bruce", tftWidth / 2, 10, 1);
    tft.setTextSize(FP);
    tft.drawCentreString(BRUCE_VERSION, tftWidth / 2, 25, 1);
    tft.setTextSize(FM);
    tft.drawCentreString(
        "PREDATORY FIRMWARE", tftWidth / 2, tftHeight + 2, 1
    ); // will draw outside the screen on non touch devices
```
и замени на:
```cpp
    tft.drawCentreString("nullcore", tftWidth / 2, 10, 1);
    tft.setTextSize(FP);
    tft.drawCentreString(BRUCE_VERSION, tftWidth / 2, 25, 1);
```

## 4. Собери
```
pip install platformio   # если нет
pio run -e nullcore
```

## 5. Сделай единый .bin для веб-флешера esp.huhn.me
Из .pio/build/nullcore/ собери merged-бинарник:
```
esptool.py --chip esp32s3 merge_bin -o nullcore_merged.bin \
  --flash_mode dio --flash_size 16MB \
  0x0 .pio/build/nullcore/bootloader.bin \
  0x8000 .pio/build/nullcore/partitions.bin \
  0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/nullcore/firmware.bin
```
(проверь реальные смещения под ESP32-S3, если esptool ругнётся)

## 6. Выдай мне путь к nullcore_merged.bin
Его я залью на устройство через https://esp.huhn.me (offset 0x0).

Если сборка упадёт с ошибкой — покажи ошибку и предложи фикс, не молчи.
