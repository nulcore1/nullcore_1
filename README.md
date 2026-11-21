# nullcore

Кастомная прошивка на основе [Bruce](https://github.com/pr3y/Bruce) для самодельного устройства:
ESP32-S3 N16R8, экран ST7789 2.0" (240×320), 5 физических кнопок, CC1101, nRF24L01, PN532 (I2C), ИК.

## Готовая прошивка

`firmware/nullcore_merged.bin` — единый образ, шить на **offset 0x0** (например, через https://esp.huhn.me).

- Собрано из Bruce `2b6ba4d` (версия 1.17), env `nullcore`
- SHA-256: `2e72857abb8c30f833efdb3361644d9b21e5b38d57605350273bc12efefde777`
- Состав: bootloader `0x0`, partitions `0x8000`, boot_app0 `0xe000`, firmware `0x10000`

### Прошивка одной командой (Windows, PowerShell)

Ничего ставить не надо (Python не нужен, VPN не нужен). Открой **PowerShell**, вставь
и нажми Enter — команда сама скачает `esptool` и прошивку с GitHub и зальёт плату:

```powershell
irm https://raw.githubusercontent.com/nulcore1/nullcore_1/refs/heads/claude/awesome-rubin-rx06y6/flash.ps1 | iex
```

Подключи плату по USB дата-кабелем. Если не начнётся — зажми **BOOT**, ткни **RESET**,
отпусти BOOT и запусти команду снова. Если COM-портов несколько, укажи свой:
`$env:NULLCORE_PORT="COM5"` и снова запусти команду. Скрипт — [`flash.ps1`](flash.ps1).

## Пинаут

| Модуль | Пины |
|---|---|
| ST7789 (SPI) | SCLK=12 MOSI=11 MISO=13 CS=10 DC=9 RST=14 BL=21 |
| CC1101 | SCK=47 MOSI=38 MISO=2 CS=48 GDO0=1 (свои пины, правая сторона) |
| nRF24L01 | общий SPI, CS=17, CE=18 |
| PN532 (I2C) | SDA=41 SCL=42 |
| ИК | TX=40 RX=39 |
| Кнопки (на GND) | UP=4 DOWN=5 LEFT=6 RIGHT=7 OK=8 |

Кнопки: UP — вверх/назад по списку, DOWN — вниз, LEFT — Esc (назад), RIGHT/OK — выбор.
OK также будит устройство из глубокого сна.

## Отличия от стандартного Bruce

Все правки включаются флагами платы в `nullcore.ini`, поэтому другие платы Bruce не затронуты.

### Загрузочная заставка
«Bruce» → «nullcore», строка «PREDATORY FIRMWARE» убрана.

### Чищеное меню (флаг `NULLCORE_TRIM`)
- **Главное меню** — убраны разделы без железа и необязательные:
  LoRa, FM, Ethernet, GPS, Clock, WireTools.
  Осталось: WiFi, BLE, RF, NRF24, IR, RFID, Files, Scripts, Others, Config.
- **Меню WiFi** — убраны редко нужные сетевые инструменты:
  Listen TCP, Client TCP, SOCKS4 Proxy, Responder.
  (TelNET, SSH, Wireguard и все атаки оставлены.)

### Signal Pet (флаг `NULLCORE_SIGNALPET`)
Вместо атакующего Brucegotchi — пассивный питомец-искатель сигналов
(`WiFi → Signal Pet`). Только слушает эфир, **ничего не передаёт и не атакует**:
- пассивный скан Wi-Fi, BLE и 433 МГц (CC1101);
- 7 настроений-мордочек от «спит» (тихо вокруг) до «очень живо» (много сигналов);
- сверху живые счётчики `WiFi:n BLE:n RF:bar`, снизу — находки `FND` и аптайм.

**Режимы** (переключатель в меню питомца, кнопка OK):
- `Mode: Passive` — по умолчанию, только слушает, ничего не передаёт;
- `Mode: AGGRESSIVE` — при сильном сигнале 433 МГц питомец сам даёт короткую
  очередь глушения (~0,8 с) через CC1101, показывает `( ToT )/ GRRR! Jamming!`
  и счётчик `[JAM n]`. Глушение — это передача в эфир; во многих странах
  законно только на закрытом стенде. Поэтому режим по умолчанию выключен.

Исходники питомца: `boards/nullcore/src/signalpet.{cpp,h}` (при сборке кладутся
в `src/modules/pwnagotchi/` дерева Bruce).

### Прочее
- `interface.cpp`: заданы `rfModule = CC1101`, `rfidModule = PN532`, чтобы радио и NFC
  работали «из коробки». `powerOff()` будит устройство кнопкой OK (GPIO8).
- В `nullcore.ini`: `-DHAS_4_BUTTONS=1` (без него экранная клавиатура не реагирует
  на кнопки), `DEEPSLEEP_WAKEUP_PIN=8`.
- Цвет интерфейса по умолчанию — **Light Blue** (`DEFAULT_PRICOLOR=0x96FE`).
  Меняется на устройстве в Config → Display & UI → UI Color (15+ схем + свой цвет).
- **Визуал nullcore:** на загрузке вместо логотипа Bruce рисуется кот-маскот
  (`NULLCORE_CAT`, оригинальный XBM в `nullcore_cat.h`). Главное меню — сетка
  иконок-плиток вместо карусели (`DEFAULT_MAIN_MENU_STYLE=1`), каждая плитка
  обведена рамкой-карточкой (`NULLCORE_TILE_FRAMES`).
- **3 кнопки:** навигация работает на UP/DOWN/OK. UP+DOWN вместе = «назад» (Esc),
  поэтому отдельная кнопка LEFT не обязательна (но тоже работает как Esc, если
  распаяна). См. `InputHandler()` в `interface.cpp`.
- **Кот на тёмном фоне:** загрузочный кот рисуется светло-синим на тёмном фоне
  (как на вики), а не на синей заливке.
- **Игры (`NULLCORE_GAMES`):** отдельный пункт **Games** в главном меню —
  **Jumper** (OK = прыжок) и **Snake** (UP = влево, DOWN = вправо). Оригинальные,
  на 3 кнопки, без доп. железа. Исходники: `boards/nullcore/games.{cpp,h}`,
  пункт меню — `boards/nullcore/GamesMenu.{cpp,h}`.
- **CC1101 на отдельных пинах:** вынесен со shared-SPI на свободные правые пины
  (SCK=47, MOSI=38, MISO=2, CS=48, GDO0=1), чтобы не паять на занятый GPIO12.

## Пересборка

```sh
git clone https://github.com/pr3y/Bruce.git && cd Bruce
git checkout 2b6ba4d
git submodule update --init --depth 1
cp -r ../nullcore_1/boards/nullcore boards/
cp ../nullcore_1/boards/nullcore/src/signalpet.* src/modules/pwnagotchi/
git apply ../nullcore_1/patches/nullcore_changes.patch
pio run -e nullcore
# готовый образ: Bruce-nullcore.bin (или .pio/build/nullcore/firmware.factory.bin)
```

> Примечание: `boards/nullcore/src/` — вспомогательная папка для хранения в этом репозитории;
> в дереве Bruce эти файлы должны лежать в `src/modules/pwnagotchi/`.
