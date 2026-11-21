// ============================================================================
//  Signal Pet — passive, receive-only signal-hunting pet. See signalpet.h.
// ============================================================================
#if !defined(LITE_VERSION)
#include "signalpet.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include <WiFi.h>
#include <globals.h>

#if defined(USE_NIMBLE) || __has_include(<NimBLEDevice.h>)
#include <NimBLEDevice.h>
#define SIGNALPET_HAS_BLE 1
#endif

#include "modules/rf/rf_utils.h"

// ---------------------------------------------------------------------------
//  Faces — signal-hunting themed. Index = mood tier.
// ---------------------------------------------------------------------------
enum PetMood : uint8_t { SLEEP, BORED, CALM, CURIOUS, HAPPY, EXCITED, ALERT, ATTACK, N_MOODS };

static const char *petFaces[N_MOODS] = {
    "(-_-) zzz", // SLEEP   - nothing around
    "(._. )",    // BORED   - almost nothing
    "( o_o)",    // CALM    - a little
    "( O_O)?",   // CURIOUS - some signals
    "(^_^)",     // HAPPY   - busy band
    "(>o<)!",    // EXCITED - very busy
    "(@_@)!!",   // ALERT   - new strong signal
    "( ToT )/",  // ATTACK  - aggressive mode: jamming burst
};

static const char *petPhrases[N_MOODS] = {
    "Quiet... napping.",
    "So empty here.",
    "A few whispers.",
    "What's that signal?",
    "Lots of chatter!",
    "The air is ALIVE!",
    "New strong signal!",
    "GRRR! Jamming!",
};

// ---------------------------------------------------------------------------
//  State
// ---------------------------------------------------------------------------
struct PetState {
    int wifiCount = 0;
    int wifiBest = -100; // strongest RSSI seen this sweep
    int bleCount = 0;
    int bleBest = -100;
    int rfLevel = -100; // peak CC1101 RSSI this sweep
    bool rfActive = false;

    int peakWifi = 0;
    int peakBle = 0;
    int rfHits = 0;
    uint32_t discoveries = 0; // times a new record was hit

    bool senseRF = true;  // CC1101 sweep enabled
    bool senseBLE = true; // BLE sweep enabled
    bool aggressive = false; // aggressive mode: jam a strong 433 MHz signal when found
    uint32_t jamCount = 0;   // how many jam bursts fired this session
    uint8_t mood = CALM;
};

static PetState pet;
static bool petExit = false;
static bool bleReady = false;

// Layout
static int sw, sh, cx, topH, botH;

static void setExit(bool v) { petExit = v; }

// ---------------------------------------------------------------------------
//  Drawing
// ---------------------------------------------------------------------------
static void drawBar(int x, int y, int w, int h, int val, int vmin, int vmax) {
    tft.drawRect(x, y, w, h, bruceConfig.priColor);
    int fill = 0;
    if (val > vmin) {
        fill = (long)(val - vmin) * (w - 2) / (vmax - vmin);
        if (fill < 0) fill = 0;
        if (fill > w - 2) fill = w - 2;
    }
    if (fill > 0) tft.fillRect(x + 1, y + 1, fill, h - 2, bruceConfig.priColor);
}

static void drawHeader() {
    tft.fillRect(0, 0, sw, topH, bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setTextDatum(TL_DATUM);

    char line[40];
    snprintf(line, sizeof(line), "WiFi:%d  BLE:%d", pet.wifiCount, pet.bleCount);
    tft.drawString(line, 2, 2);

    // RF activity bar on the right half of the header
    if (pet.senseRF && bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        int bw = sw / 3;
        tft.drawString("RF", sw - bw - 18, 2);
        drawBar(sw - bw - 2, 2, bw, 8, pet.rfLevel, -95, -30);
    }
    tft.drawLine(0, topH - 1, sw, topH - 1, bruceConfig.priColor);
}

static void drawFooter() {
    tft.fillRect(0, botH, sw, sh - botH, bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);

    uint32_t up = millis() / 1000;
    char l[56];
    if (pet.aggressive) {
        snprintf(
            l, sizeof(l), "[JAM %lu] UP %02u:%02u:%02u", (unsigned long)pet.jamCount,
            (unsigned)(up / 3600), (unsigned)((up % 3600) / 60), (unsigned)(up % 60)
        );
    } else {
        snprintf(
            l, sizeof(l), "FND %lu  UP %02u:%02u:%02u", (unsigned long)pet.discoveries,
            (unsigned)(up / 3600), (unsigned)((up % 3600) / 60), (unsigned)(up % 60)
        );
    }
    tft.setTextDatum(TL_DATUM);
    tft.drawString(l, 2, botH + 4);

    int bat = getBattery();
    if (bat > 0 && bat <= 100) {
        char b[12];
        snprintf(b, sizeof(b), "BAT %d%%", bat);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(b, sw - 2, botH + 4);
    }
    tft.drawLine(0, botH, sw, botH, bruceConfig.priColor);
}

static void drawFace() {
    tft.fillRect(0, topH + 1, sw, botH - topH - 1, bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(FG);
    tft.drawCentreString(petFaces[pet.mood], cx, (topH + botH) / 2 - 18, 1);
    tft.setTextSize(FM);
    tft.drawCentreString(petPhrases[pet.mood], cx, botH - 26, 1);
}

static void drawAll() {
    drawHeader();
    drawFace();
    drawFooter();
}

// ---------------------------------------------------------------------------
//  Mood logic
// ---------------------------------------------------------------------------
static void computeMood(bool newRecord) {
    if (newRecord) {
        pet.mood = ALERT;
        return;
    }
    int score = pet.wifiCount + pet.bleCount + (pet.rfActive ? 4 : 0);
    if (score == 0) pet.mood = SLEEP;
    else if (score <= 1) pet.mood = BORED;
    else if (score <= 4) pet.mood = CALM;
    else if (score <= 8) pet.mood = CURIOUS;
    else if (score <= 15) pet.mood = HAPPY;
    else pet.mood = EXCITED;
}

// ---------------------------------------------------------------------------
//  Senses (all receive-only)
// ---------------------------------------------------------------------------
#if defined(NULLCORE_QEMU_MIRROR)
// Emulator has no real radios; fake a lively environment so the UI can be exercised.
#include <esp_random.h>
static void senseWifi() { pet.wifiCount = esp_random() % 9; pet.wifiBest = -40 - (int)(esp_random() % 50); }
static void senseBle() { pet.bleCount = pet.senseBLE ? (esp_random() % 6) : 0; }
static void senseRf() {
    pet.rfActive = pet.senseRF && (esp_random() % 3 == 0);
    pet.rfLevel = pet.rfActive ? -40 - (int)(esp_random() % 20) : -95;
}
#else
static void senseWifi() {
    // Passive scan: listen on every channel, never transmit.
    int n = WiFi.scanNetworks(/*async*/ false, /*show_hidden*/ true, /*passive*/ false, 90);
    if (n < 0) n = 0;
    pet.wifiCount = n;
    pet.wifiBest = -100;
    for (int i = 0; i < n; i++) {
        int r = WiFi.RSSI(i);
        if (r > pet.wifiBest) pet.wifiBest = r;
    }
    WiFi.scanDelete();
}

#if defined(SIGNALPET_HAS_BLE)
static void senseBle() {
    if (!pet.senseBLE) {
        pet.bleCount = 0;
        return;
    }
    if (!bleReady) {
        if (!NimBLEDevice::isInitialized()) NimBLEDevice::init("");
        bleReady = true;
    }
    NimBLEScan *scan = NimBLEDevice::getScan();
    if (!scan) {
        pet.bleCount = 0;
        return;
    }
    scan->setActiveScan(false); // passive: do not send scan requests
    scan->setInterval(97);
    scan->setWindow(67);
    NimBLEScanResults res = scan->getResults(1400, false);
    pet.bleCount = res.getCount();
    pet.bleBest = -100;
    for (int i = 0; i < res.getCount(); i++) {
        int r = res.getDevice(i)->getRSSI();
        if (r > pet.bleBest) pet.bleBest = r;
    }
    scan->clearResults();
}
#else
static void senseBle() { pet.bleCount = 0; }
#endif

static void senseRf() {
    pet.rfActive = false;
    if (!pet.senseRF || bruceConfigPins.rfModule != CC1101_SPI_MODULE) return;
    if (!initRfModule("rx", bruceConfigPins.rfFreq)) return;
    int best = -120;
    for (int i = 0; i < 12; i++) {
        int r = ELECHOUSE_cc1101.getRssi();
        if (r > best) best = r;
        delay(4);
    }
    deinitRfModule();
    tft.drawPixel(0, 0, 0); // CC1101 shares SPI with the TFT — nudge the bus back
    pet.rfLevel = best;
    pet.rfActive = best > -75; // real signal above the ambient noise floor
}
#endif // NULLCORE_QEMU_MIRROR

// ---------------------------------------------------------------------------
//  Aggressive mode — fire a short jamming burst on the 433 MHz band the pet is
//  tuned to. This TRANSMITS: only reachable after the user flips the pet into
//  "Aggressive" mode from the menu (default is the passive, receive-only mode).
// ---------------------------------------------------------------------------
static void jamBurst() {
    pet.mood = ATTACK;
    drawFace();
    pet.jamCount++;
#if !defined(NULLCORE_QEMU_MIRROR)
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) return;
    if (!initRfModule("tx", bruceConfigPins.rfFreq)) return;
    int pin = (int)bruceConfigPins.CC1101_bus.io0; // CC1101 GDO0 drives OOK output
    uint32_t start = millis();
    while (millis() - start < 800) { // ~0.8 s burst, then back to listening
        for (int i = 0; i < 100; i++) {
            digitalWrite(pin, HIGH);
            delayMicroseconds(3);
            digitalWrite(pin, LOW);
            delayMicroseconds(1);
        }
        if (check(EscPress)) break;
    }
    digitalWrite(pin, LOW);
    deinitRfModule();
    tft.drawPixel(0, 0, 0); // CC1101 shares SPI with the TFT — nudge the bus back
#else
    delay(600);
#endif
}

// ---------------------------------------------------------------------------
//  Options menu (Sel)
// ---------------------------------------------------------------------------
static void petMenu() {
    String md = pet.aggressive ? "Mode: AGGRESSIVE" : "Mode: Passive";
    String rf = pet.senseRF ? "RF sense: ON" : "RF sense: OFF";
    String bl = pet.senseBLE ? "BLE sense: ON" : "BLE sense: OFF";
    options = {
        {md.c_str(), []() { pet.aggressive = !pet.aggressive; }},
        {rf.c_str(), []() { pet.senseRF = !pet.senseRF; }},
        {bl.c_str(), []() { pet.senseBLE = !pet.senseBLE; }},
        {"Main Menu", []() { setExit(true); }},
    };
    loopOptions(options);
}

// ---------------------------------------------------------------------------
//  Entry point
// ---------------------------------------------------------------------------
void signalpet_start() {
    petExit = false;
    pet = PetState();

    sw = tftWidth;
    sh = tftHeight;
    cx = sw / 2;
    topH = sh / 10;
    if (topH < 12) topH = 12;
    botH = sh - topH;

    // Draw the UI first so the pet is on screen instantly, before any (slow) radio init.
    tft.fillScreen(bruceConfig.bgColor);
    pet.mood = CALM;
    drawAll();

#if !defined(NULLCORE_QEMU_MIRROR)
    // Wi-Fi in station mode, not connected: lets us scan without joining anything.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, true);
#endif

    uint8_t which = 0; // rotate senses so the face stays responsive between scans
    uint32_t lastSweep = 0;

    while (true) {
        if (check(EscPress) || petExit) break;
        if (check(SelPress)) {
            petMenu();
            if (petExit) break;
            tft.fillScreen(bruceConfig.bgColor);
            drawAll();
        }

        if (millis() - lastSweep > 900) {
            lastSweep = millis();
            bool newRecord = false;

            if (which == 0) {
                senseWifi();
                if (pet.wifiCount > pet.peakWifi) {
                    pet.peakWifi = pet.wifiCount;
                    newRecord = true;
                }
            } else if (which == 1) {
                senseBle();
                if (pet.bleCount > pet.peakBle) {
                    pet.peakBle = pet.bleCount;
                    newRecord = true;
                }
            } else {
                senseRf();
                if (pet.rfActive) {
                    pet.rfHits++;
                    newRecord = true;
                }
            }
            which = (which + 1) % 3;

            if (newRecord) pet.discoveries++;
            computeMood(newRecord);
            drawAll();

            // Aggressive mode: when a strong 433 MHz signal was just sensed, bite back
            // with a short jamming burst on that band. Passive mode never transmits.
            if (pet.aggressive && pet.rfActive) {
                jamBurst();
                drawAll();
            } else if (newRecord) {
                vTaskDelay(700 / portTICK_PERIOD_MS); // let the alert face linger
            }
        }

        vTaskDelay(30 / portTICK_PERIOD_MS);
    }

    // Cleanup — stop every radio we touched
    WiFi.scanDelete();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
#if defined(SIGNALPET_HAS_BLE)
    if (bleReady) {
        NimBLEDevice::deinit(true);
        bleReady = false;
    }
#endif
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) deinitRfModule();
}
#endif
