# ============================================================
#  nullcore — автоматический прошивальщик для Windows
#  Запуск одной командой (ничего ставить не надо, Python не нужен):
#
#    irm https://raw.githubusercontent.com/nulcore1/nullcore_1/refs/heads/claude/awesome-rubin-rx06y6/flash.ps1 | iex
#
#  Качает esptool.exe и прошивку nullcore с GitHub и шьёт ESP32-S3 на 0x0.
# ============================================================

$ErrorActionPreference = 'Stop'
$ProgressPreference     = 'SilentlyContinue'   # быстрее Invoke-WebRequest
try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}

$FW_URL  = 'https://raw.githubusercontent.com/nulcore1/nullcore_1/refs/heads/claude/awesome-rubin-rx06y6/firmware/nullcore_merged.bin'
$ESP_VER = 'v4.8.1'
$ESP_URL = "https://github.com/espressif/esptool/releases/download/$ESP_VER/esptool-$ESP_VER-win64.zip"

$work = Join-Path $env:TEMP 'nullcore'
New-Item -Force -ItemType Directory $work | Out-Null

Write-Host ''
Write-Host '==============================' -ForegroundColor Cyan
Write-Host '   nullcore flasher (ESP32-S3)' -ForegroundColor Cyan
Write-Host '==============================' -ForegroundColor Cyan
Write-Host ''

# --- 1. esptool -------------------------------------------------------------
$esptool = Join-Path $work 'esptool-win64\esptool.exe'
if (-not (Test-Path $esptool)) {
    Write-Host '[1/3] Качаю esptool (~33 МБ)...' -ForegroundColor White
    $zip = Join-Path $work 'esptool.zip'
    Invoke-WebRequest $ESP_URL -OutFile $zip
    Expand-Archive -Force $zip $work
}
if (-not (Test-Path $esptool)) {
    $esptool = (Get-ChildItem $work -Recurse -Filter esptool.exe | Select-Object -First 1).FullName
}
if (-not $esptool) { throw 'esptool.exe не найден после распаковки' }
Write-Host '      esptool готов.' -ForegroundColor DarkGray

# --- 2. прошивка ------------------------------------------------------------
Write-Host '[2/3] Качаю прошивку nullcore (~4 МБ)...' -ForegroundColor White
$fw = Join-Path $work 'nullcore_merged.bin'
Invoke-WebRequest $FW_URL -OutFile $fw
Write-Host "      $([math]::Round((Get-Item $fw).Length/1MB,2)) МБ скачано." -ForegroundColor DarkGray

# --- 3. прошивка платы ------------------------------------------------------
Write-Host ''
Write-Host '[3/3] Подключи плату по USB.' -ForegroundColor Yellow
Write-Host '      Если не начнётся — зажми BOOT, ткни RESET, отпусти BOOT.' -ForegroundColor Yellow
Write-Host ''

# Если задан $env:NULLCORE_PORT=COMx — шьём именно на него, иначе автопоиск.
$portArgs = @()
if ($env:NULLCORE_PORT) { $portArgs = @('--port', $env:NULLCORE_PORT) }

& $esptool @portArgs --chip esp32s3 --baud 921600 --before default_reset --after hard_reset write_flash 0x0 $fw
$flashExit = $LASTEXITCODE

# --- чистка временных файлов -----------------------------------------------
Write-Host ''
Write-Host 'Убираю временные файлы...' -ForegroundColor DarkGray
try { Remove-Item -Recurse -Force $work -ErrorAction Stop } catch {}

if ($flashExit -ne 0) {
    Write-Host ''
    Write-Host 'Не удалось прошить.' -ForegroundColor Red
    Write-Host 'Подсказки:' -ForegroundColor Red
    Write-Host '  * Зажми BOOT, ткни RESET, отпусти BOOT и запусти команду снова.' -ForegroundColor Red
    Write-Host '  * Если портов несколько, укажи свой:  $env:NULLCORE_PORT="COM5"  и снова запусти команду.' -ForegroundColor Red
    Write-Host '  * Проверь, что это дата-кабель (не только зарядка) и стоят драйверы USB-CDC.' -ForegroundColor Red
    exit 1
}

Write-Host ''
Write-Host '==> Готово! Временные файлы удалены.' -ForegroundColor Green
Write-Host '    Отключи и снова включи плату — загрузится nullcore (кот на старте).' -ForegroundColor Green
Write-Host ''
