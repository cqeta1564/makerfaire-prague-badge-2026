#pragma once

#include <Arduino.h>

#ifndef MAKER_BADGE_REV_A_DISPLAY
#define MAKER_BADGE_REV_A_DISPLAY 0
#endif

constexpr uint32_t SERIAL_BAUD = 115200;

constexpr uint8_t PIN_EPD_BUSY = 42;
constexpr uint8_t PIN_EPD_RST = 39;
constexpr uint8_t PIN_EPD_DC = 40;
constexpr uint8_t PIN_EPD_CS = 41;
constexpr uint8_t PIN_EPD_POWER = 16;

constexpr uint8_t PIN_BOOT_BUTTON = 0;
constexpr uint8_t PIN_UNUSED = 255;
constexpr uint8_t PIN_TOUCH_PAIR_1 = 1;
constexpr uint8_t PIN_TOUCH_PAIR_2 = 2;
constexpr uint8_t PIN_TOUCH_UNUSED_3 = 3;
constexpr uint8_t PIN_TOUCH_SHOW_1 = 4;
constexpr uint8_t PIN_TOUCH_SHOW_2 = 5;

constexpr uint8_t PIN_RGB_LED = 18;
constexpr uint8_t PIN_NEOPIXEL_PWR = 21;
constexpr uint8_t NEOPIXEL_COUNT = 4;

#ifndef PIN_IR_TX
#define PIN_IR_TX 13
#endif

#ifndef PIN_IR_RX
#define PIN_IR_RX 7
#endif

#ifndef PIN_IR_RX_POWER
#define PIN_IR_RX_POWER PIN_NEOPIXEL_PWR
#endif

#ifndef IR_RX_POWER_ACTIVE_LOW
#define IR_RX_POWER_ACTIVE_LOW 1
#endif

#ifndef TOUCH_ACTIVE_HIGH
#define TOUCH_ACTIVE_HIGH 1
#endif

#ifndef TOUCH_THRESHOLD
#define TOUCH_THRESHOLD 25000
#endif

#ifndef SLEEP_AFTER_ACTION
#define SLEEP_AFTER_ACTION 1
#endif

#ifndef IDLE_SLEEP_TIMEOUT_MS
#define IDLE_SLEEP_TIMEOUT_MS 60000UL
#endif

constexpr uint8_t STORAGE_MAGIC = 0x5b;
constexpr uint8_t STORAGE_MAGIC2 = 42;
constexpr uint16_t STORAGE_COUNT = 512;
constexpr uint16_t STORAGE_MAX_ID = 4096;
constexpr uint16_t STORAGE_WILDCARD_ID = STORAGE_MAX_ID - 1;
constexpr uint8_t WEB_BADGE_CONFIG_SUBTYPE = 0x40;
constexpr uint32_t WEB_BADGE_CONFIG_OFFSET = 0x3F0000;
constexpr uint32_t WEB_BADGE_CONFIG_SIZE = 0x10000;

constexpr uint8_t STORAGE_MAX_TEAM = 4;
constexpr uint8_t STORAGE_TEAM_UNDECIDED = 0;
constexpr uint8_t STORAGE_TEAM_RED = 1;
constexpr uint8_t STORAGE_TEAM_GREEN = 2;
constexpr uint8_t STORAGE_TEAM_BLUE = 3;

constexpr uint16_t PIXELS_ALL_RED = 0x492;
constexpr uint16_t PIXELS_ALL_GREEN = 0x924;
constexpr uint16_t PIXELS_ALL_BLUE = 0x249;
