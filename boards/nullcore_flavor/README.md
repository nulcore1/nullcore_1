# nullcore flavor — начинка nullcore для готовых плат

Переносит «вид и фишки» nullcore на **штатные платы Bruce** (T-Embed, M5StickC,
Cardputer, T-Deck, CYD, ESP32-S3 и т.д.), **не трогая их железо/пины/кнопки**.

Добавляется:
- 🐈 **кот** на загрузке вместо лого Bruce (`NULLCORE_CAT`);
- ▦ **меню-сетка** плитками вместо карусели (`DEFAULT_MAIN_MENU_STYLE=1`) с рамками (`NULLCORE_TILE_FRAMES`);
- 🎮 **Games** — Jumper и Snake (`NULLCORE_GAMES`), управление штатными кнопками платы;
- 🎨 тема **Light Blue** по умолчанию (`DEFAULT_PRICOLOR`).

НЕ добавляется (это только для DIY-платы nullcore): урезание меню, Signal Pet,
раскладка 3 кнопок, пины радио — всё это остаётся как у самой платы.

## Сборка

```sh
pio run -e nullcore-<плата>
```

Готовые env-ы:

| Плата | env |
|---|---|
| LilyGO T-Embed | `nullcore-t-embed` |
| LilyGO T-Embed CC1101 | `nullcore-t-embed-cc1101` |
| LilyGO T-Display-S3 | `nullcore-t-display-s3` |
| LilyGO T-Deck | `nullcore-t-deck` |
| M5StickC Plus | `nullcore-m5stickc-plus` |
| M5StickC Plus2 | `nullcore-m5stickc-plus2` |
| M5Stick S3 | `nullcore-m5stick-s3` |
| M5Cardputer | `nullcore-cardputer` |
| ESP32-S3 DevKitC-1 | `nullcore-s3-devkit` |
| CYD 2432S028 | `nullcore-cyd-2432s028` |

## Добавить свою плату

В `nullcore_flavor.ini` скопируй блок и подставь имя базового env Bruce
(полный список: `pio project config`):

```ini
[env:nullcore-МОЯ-ПЛАТА]
extends = env:БАЗОВЫЙ-ENV
build_flags = ${env:БАЗОВЫЙ-ENV.build_flags} ${nullcore_flavor.build_flags}
build_src_filter = ${env:БАЗОВЫЙ-ENV.build_src_filter} ${nullcore_flavor.src_filter}
```

Прошивка — как обычно в Bruce для этой платы (merged `.bin` на `0x0` или штатный аплоадер).
