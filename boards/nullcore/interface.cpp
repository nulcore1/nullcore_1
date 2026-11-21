// ============================================================
//  nullcore — button-driven interface (no touchscreen)
//  5 buttons wired to GND, using the ESP32 internal pull-ups.
//
//  UP    = GPIO4   -> Prev  (move up / left in menus)
//  DOWN  = GPIO5   -> Next  (move down / right in menus)
//  LEFT  = GPIO6   -> Esc   (back / exit)
//  RIGHT = GPIO7   -> Sel   (enter / confirm)
//  OK    = GPIO8   -> Sel   (enter / confirm)
// ============================================================
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include <driver/rtc_io.h>
#include <interface.h>
#ifdef NULLCORE_QEMU_MIRROR
#include <globals.h>
#endif

#ifndef BTN_ACT
#define BTN_ACT LOW // buttons pull the pin to GND when pressed
#endif

#define BTN_UP 4
#define BTN_DOWN 5
#define BTN_LEFT 6
#define BTN_RIGHT 7
#define BTN_OK 8

/***************************************************************************************
** _setup_gpio() — first stage GPIO setup (runs before tft.init())
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.rotation = 1;

    // Infrared
    bruceConfigPins.irTx = 40;
    bruceConfigPins.irRx = 39;

    // PN532 NFC over I2C (SDA=41, SCL=42)
    bruceConfigPins.i2c_bus = {(gpio_num_t)41, (gpio_num_t)42};

    // CC1101 Sub-GHz: sck, miso, mosi, cs, gdo0
    // nullcore: moved onto the free right-side header pins so the crowded
    // left side / shared display SCK (GPIO12) never has to be touched.
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)47, (gpio_num_t)2, (gpio_num_t)38, (gpio_num_t)48, (gpio_num_t)1
    };

    // nRF24L01: sck, miso, mosi, cs(ss), ce
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)17, (gpio_num_t)18
    };

    // Default modules fitted on nullcore (otherwise Bruce assumes M5 RF433 / M5 RFID2)
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;

    // Generic/outer SPI bus (sck, miso, mosi, cs)
    bruceConfigPins.outer_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10};

    // Buttons — active-low with internal pull-ups
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_OK, INPUT_PULLUP);

    bruceConfig.colorInverted = 1;
}

/***************************************************************************************
** _post_setup_gpio() — second stage (runs AFTER tft.init())
***************************************************************************************/
void _post_setup_gpio() {
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
#ifdef NULLCORE_QEMU_MIRROR // (globals.h declares tft)
    // Emulator test builds only: mirror every draw call over serial from boot on
    tft.startAsyncSerial();
#endif
}

void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/***************************************************************************************
** InputHandler() — sets PrevPress / NextPress / SelPress / EscPress / AnyKeyPress
***************************************************************************************/
void InputHandler(void) {
    static long lastPress = 0;
    if (millis() - lastPress < 180 && !LongPress) return;

    bool up = (digitalRead(BTN_UP) == BTN_ACT);
    bool down = (digitalRead(BTN_DOWN) == BTN_ACT);
    bool left = (digitalRead(BTN_LEFT) == BTN_ACT);
    bool right = (digitalRead(BTN_RIGHT) == BTN_ACT);
    bool ok = (digitalRead(BTN_OK) == BTN_ACT);

    if (!(up || down || left || right || ok)) return;

    checkPowerSaveTime();
    if (!wakeUpScreen()) AnyKeyPress = true;

    // 3-button layout: UP + DOWN pressed together = Esc (back), so a dedicated
    // LEFT button is optional. LEFT still works as Esc when wired.
    if (up && down) {
        EscPress = true;
    } else {
        if (up) PrevPress = true;
        if (down) NextPress = true;
    }
    if (left) EscPress = true;
    if (right || ok) SelPress = true;

    lastPress = millis();
}

void powerOff() {
    // Wake up again when OK (GPIO8, an RTC GPIO) is pressed (pulled to GND)
    rtc_gpio_pullup_en((gpio_num_t)BTN_OK);
    rtc_gpio_pulldown_dis((gpio_num_t)BTN_OK);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_OK, 0);
    esp_deep_sleep_start();
}

void checkReboot() {}
