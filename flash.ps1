# ============================================================
#  nullcore flasher — Windows / PowerShell
#  Запуск одной командой:
#    irm https://raw.githubusercontent.com/nulcore1/nullcore_1/refs/heads/main/flash.ps1 | iex
#  Качает esptool + прошивку выбранной платы с GitHub и шьёт на 0x0.
# ============================================================
$ErrorActionPreference = 'Stop'
$ProgressPreference     = 'SilentlyContinue'
try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}
try { chcp 65001 > $null } catch {}   # UTF-8, чтобы русский не ломался

$FW_BASE = 'https://raw.githubusercontent.com/nulcore1/nullcore_1/refs/heads/main/firmware/'
$ESP_VER = 'v4.8.1'
$ESP_URL = "https://github.com/espressif/esptool/releases/download/$ESP_VER/esptool-$ESP_VER-win64.zip"

$script:lang = 'ru'
function L($ru, $en) { if ($script:lang -eq 'en') { $en } else { $ru } }

function Logo {
    Clear-Host
    Write-Host ''
    Write-Host '              _ _                       ' -ForegroundColor Cyan
    Write-Host '  _ __  _   _| | | ___ ___  _ __ ___    ' -ForegroundColor Cyan
    Write-Host " | '_ \| | | | | |/ __/ _ \| '__/ _ \   " -ForegroundColor Cyan
    Write-Host ' | | | | |_| | | | (_| (_) | | |  __/   ' -ForegroundColor White
    Write-Host ' |_| |_|\__,_|_|_|\___\___/|_|  \___|   ' -ForegroundColor White
    Write-Host '        f l a s h e r  ·  ESP32         ' -ForegroundColor DarkCyan
    Write-Host ''
}

function Line { Write-Host ('-' * 44) -ForegroundColor DarkGray }

# ---- список плат: имя + файл прошивки ----
$boards = @(
    @{ n = 'DIY nullcore (своё устройство)';  f = 'nullcore_merged.bin' },
    @{ n = 'LilyGO T-Embed';                  f = 'nullcore-t-embed_merged.bin' },
    @{ n = 'LilyGO T-Embed CC1101';           f = 'nullcore-t-embed-cc1101_merged.bin' },
    @{ n = 'LilyGO T-Display-S3';             f = 'nullcore-t-display-s3_merged.bin' },
    @{ n = 'LilyGO T-Deck';                   f = 'nullcore-t-deck_merged.bin' },
    @{ n = 'M5StickC Plus';                   f = 'nullcore-m5stickc-plus_merged.bin' },
    @{ n = 'M5StickC Plus2';                  f = 'nullcore-m5stickc-plus2_merged.bin' },
    @{ n = 'M5Stick S3';                      f = 'nullcore-m5stick-s3_merged.bin' },
    @{ n = 'M5Cardputer';                     f = 'nullcore-cardputer_merged.bin' },
    @{ n = 'ESP32-S3 DevKitC-1';              f = 'nullcore-s3-devkit_merged.bin' },
    @{ n = 'CYD 2432S028';                    f = 'nullcore-cyd-2432s028_merged.bin' }
)

# ---------- 1. выбор языка ----------
Logo
Write-Host ' Выберите язык / Choose language:' -ForegroundColor Yellow
Write-Host '   [1] Русский'
Write-Host '   [2] English'
$sel = Read-Host ' >'
if ($sel -eq '2') { $script:lang = 'en' } else { $script:lang = 'ru' }

# ---------- 2. выбор платы ----------
Logo
Write-Host (L ' Выберите плату:' ' Choose your board:') -ForegroundColor Yellow
Line
for ($i = 0; $i -lt $boards.Count; $i++) {
    $num = ('{0,2}' -f ($i + 1))
    Write-Host "  [$num] " -ForegroundColor Cyan -NoNewline
    Write-Host $boards[$i].n
}
Line
$pick = Read-Host (L ' Номер платы >' ' Board number >')
$idx = 0
if (-not [int]::TryParse($pick, [ref]$idx) -or $idx -lt 1 -or $idx -gt $boards.Count) {
    Write-Host (L ' Неверный номер.' ' Invalid number.') -ForegroundColor Red; return
}
$board = $boards[$idx - 1]

Logo
Write-Host (L " Плата:    " " Board:    ") -ForegroundColor DarkGray -NoNewline
Write-Host $board.n -ForegroundColor White
Write-Host (L " Прошивка: " " Firmware: ") -ForegroundColor DarkGray -NoNewline
Write-Host $board.f -ForegroundColor White
Line

$work = Join-Path $env:TEMP 'nullcore'
New-Item -Force -ItemType Directory $work | Out-Null

# ---------- 3. esptool ----------
$esptool = Join-Path $work 'esptool-win64\esptool.exe'
if (-not (Test-Path $esptool)) {
    Write-Host (L '[1/3] Качаю esptool (~33 МБ)...' '[1/3] Downloading esptool (~33 MB)...') -ForegroundColor White
    $zip = Join-Path $work 'esptool.zip'
    Invoke-WebRequest $ESP_URL -OutFile $zip
    Expand-Archive -Force $zip $work
}
if (-not (Test-Path $esptool)) {
    $esptool = (Get-ChildItem $work -Recurse -Filter esptool.exe | Select-Object -First 1).FullName
}
if (-not $esptool) { Write-Host (L 'esptool не найден' 'esptool not found') -ForegroundColor Red; return }
Write-Host (L '      esptool готов.' '      esptool ready.') -ForegroundColor DarkGray

# ---------- 4. прошивка ----------
Write-Host (L '[2/3] Качаю прошивку...' '[2/3] Downloading firmware...') -ForegroundColor White
$fw = Join-Path $work $board.f
Invoke-WebRequest ($FW_BASE + $board.f) -OutFile $fw
Write-Host ("      {0} MB" -f [math]::Round((Get-Item $fw).Length / 1MB, 2)) -ForegroundColor DarkGray

# ---------- 5. шьём ----------
Write-Host ''
Write-Host (L '[3/3] Подключи плату по USB дата-кабелем.' '[3/3] Connect the board by USB (data cable).') -ForegroundColor Yellow
Write-Host (L '      Если не пойдёт — зажми BOOT, ткни RESET, отпусти BOOT.' '      If it fails - hold BOOT, tap RESET, release BOOT.') -ForegroundColor Yellow
Write-Host ''

$portArgs = @()
if ($env:NULLCORE_PORT) { $portArgs = @('--port', $env:NULLCORE_PORT) }

& $esptool @portArgs --baud 921600 --before default_reset --after hard_reset write_flash 0x0 $fw
$flashExit = $LASTEXITCODE

Write-Host ''
Write-Host (L 'Убираю временные файлы...' 'Cleaning up...') -ForegroundColor DarkGray
try { Remove-Item -Recurse -Force $work -ErrorAction Stop } catch {}

if ($flashExit -ne 0) {
    Write-Host ''
    Write-Host (L 'Не удалось прошить.' 'Flashing failed.') -ForegroundColor Red
    Write-Host (L '  * Зажми BOOT, ткни RESET, отпусти BOOT и запусти снова.' '  * Hold BOOT, tap RESET, release BOOT, run again.') -ForegroundColor Red
    Write-Host (L '  * Несколько портов: $env:NULLCORE_PORT="COM5" и снова запусти.' '  * Several ports: $env:NULLCORE_PORT="COM5" then run again.') -ForegroundColor Red
    return
}

Write-Host ''
Write-Host (L '==> Готово! Временные файлы удалены.' '==> Done! Temp files removed.') -ForegroundColor Green
Write-Host (L '    Перезагрузи плату — загрузится nullcore.' '    Reboot the board - nullcore will start.') -ForegroundColor Green
Write-Host ''
