# nullcore

Кастомная прошивка на основе [Bruce](https://github.com/pr3y/Bruce) для самодельного устройства:
ESP32-S3 N16R8, экран ST7789 2.0" (240×320), 5 физических кнопок, CC1101, nRF24L01, PN532 (I2C), ИК.

## Готовая прошивка

`firmware/nullcore_merged.bin` — единый образ, шить на **offset 0x0** (например, через https://esp.huhn.me).

- Собрано из Bruce `2b6ba4d` (версия 1.17), env `nullcore`
- SHA-256: `a6f82e4fcf49d75ee82a4b965c9cfe1fc93c05f93121dc1d629c599805a1d8d5`
- Состав: bootloader `0x0`, partitions `0x8000`, boot_app0 `0xe000`, firmware `0x10000`

## Пинаут

| Модуль | Пины |
|---|---|
| ST7789 (SPI) | SCLK=12 MOSI=11 MISO=13 CS=10 DC=9 RST=14 BL=21 |
| CC1101 | общий SPI, CS=15, GDO0=16 |
| nRF24L01 | общий SPI, CS=17, CE=18 |
| PN532 (I2C) | SDA=41 SCL=42 |
| ИК | TX=40 RX=39 |
| Кнопки (на GND) | UP=4 DOWN=5 LEFT=6 RIGHT=7 OK=8 |

Кнопки: UP — вверх/назад по списку, DOWN — вниз, LEFT — Esc (назад), RIGHT/OK — выбор.
OK также будит устройство из глубокого сна.

## Пересборка

```sh
git clone https://github.com/pr3y/Bruce.git && cd Bruce
git submodule update --init --depth 1
cp -r ../nullcore_1/boards/nullcore boards/
git apply ../nullcore_1/patches/boot_screen_nullcore.patch
pio run -e nullcore
# готовый образ: Bruce-nullcore.bin (или .pio/build/nullcore/firmware.factory.bin)
```

## Отличия от исходных файлов из задания

- `nullcore.ini`: добавлен `-DHAS_4_BUTTONS=1` — без флага раскладки кнопок экранная клавиатура
  Bruce (ввод паролей Wi-Fi, имён файлов) не реагирует на кнопки. Добавлены
  `DEEPSLEEP_WAKEUP_PIN=8` / `DEEPSLEEP_PIN_ACT=LOW` для пункта «Deep Sleep».
- `interface.cpp`: `powerOff()` теперь включает пробуждение по кнопке OK (GPIO8) перед
  глубоким сном — раньше устройство можно было разбудить только сбросом.
