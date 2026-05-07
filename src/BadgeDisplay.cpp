#include "BadgeDisplay.h"

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <WS2812FX.h>

#include "BadgeConfig.h"
#include "BadgeStorage.h"

#if MAKER_BADGE_REV_A_DISPLAY
GxEPD2_BW<GxEPD2_213_T5D, GxEPD2_213_T5D::HEIGHT> display(GxEPD2_213_T5D(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
#else
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(GxEPD2_213_B74(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
#endif

namespace {
WS2812FX neopixels(NEOPIXEL_COUNT, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);

LedAnim ledAnim = LED_ANIM_OFF;
uint32_t ledAnimLastAt = 0;
uint8_t ledAnimPhase = 0;

const Rgb pixelIdColors[8] = {
  {0, 0, 0},
  {0, 0, 35},
  {35, 0, 0},
  {35, 0, 45},
  {0, 25, 0},
  {0, 25, 35},
  {35, 25, 0},
  {35, 30, 45},
};

const Rgb COLOR_DIM_RED = {28, 0, 0};
const Rgb COLOR_DIM_GREEN = {0, 30, 0};
const Rgb COLOR_DIM_BLUE = {0, 0, 28};
const Rgb COLOR_DIM_CYAN = {0, 20, 35};
const Rgb COLOR_DIM_MAGENTA = {35, 0, 35};
const Rgb COLOR_DIM_YELLOW = {35, 25, 0};
const Rgb COLOR_DIM_WHITE = {35, 30, 35};

const char *teamName(uint8_t team) {
  switch (team) {
    case STORAGE_TEAM_RED: return "red";
    case STORAGE_TEAM_GREEN: return "green";
    case STORAGE_TEAM_BLUE: return "blue";
    default: return "none";
  }
}

bool touchPinTouched(uint8_t pin) {
  uint16_t value = touchRead(pin);
#if TOUCH_ACTIVE_HIGH
  return value > TOUCH_THRESHOLD;
#else
  return value < TOUCH_THRESHOLD;
#endif
}

void drawCentered(const String &text, int16_t y, const GFXfont *font) {
  int16_t tbx;
  int16_t tby;
  uint16_t tbw;
  uint16_t tbh;

  display.setFont(font);
  display.getTextBounds(text, 0, y, &tbx, &tby, &tbw, &tbh);
  display.setCursor((display.width() - tbw) / 2 - tbx, y);
  display.print(text);
}

void drawIdBlocks(uint16_t id, int16_t x, int16_t y) {
  display.setFont(&FreeMonoBold9pt7b);
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t code = id & 0b111;
    int16_t bx = x + i * 38;
    display.drawRoundRect(bx, y, 30, 24, 3, GxEPD_BLACK);
    display.setCursor(bx + 10, y + 17);
    display.print(code);
    id >>= 3;
  }
}
}

void badgeDisplaySetup() {
  pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);
  pinMode(PIN_EPD_POWER, OUTPUT);
  pinMode(PIN_NEOPIXEL_PWR, OUTPUT);
  digitalWrite(PIN_EPD_POWER, LOW);
  digitalWrite(PIN_NEOPIXEL_PWR, LOW);

  neopixels.init();
  neopixels.setBrightness(18);
  neopixels.strip_off();

  display.init(SERIAL_BAUD);
}

void badgeDisplayPowerDown() {
  pixelsOff();
  neopixels.show();
  delay(2);

  digitalWrite(PIN_NEOPIXEL_PWR, HIGH);

  display.hibernate();
  SPI.end();
  digitalWrite(PIN_EPD_POWER, HIGH);

  digitalWrite(PIN_RGB_LED, LOW);
  pinMode(PIN_RGB_LED, INPUT);
  pinMode(PIN_EPD_BUSY, INPUT);
  pinMode(PIN_EPD_RST, INPUT);
  pinMode(PIN_EPD_DC, INPUT);
  pinMode(PIN_EPD_CS, INPUT);
}

void pixelsOff() {
  ledAnim = LED_ANIM_OFF;
  neopixels.strip_off();
}

void pixelsSetAll(Rgb color) {
  for (uint8_t i = 0; i < NEOPIXEL_COUNT; i++) {
    neopixels.setPixelColor(i, color.r, color.g, color.b);
  }
  neopixels.show();
}

void pixelsShowId(uint16_t id) {
  ledAnim = LED_ANIM_OFF;

  for (uint8_t i = 0; i < NEOPIXEL_COUNT; i++) {
    Rgb color = pixelIdColors[id & 0b111];
    neopixels.setPixelColor(i, color.r, color.g, color.b);
    id >>= 3;
  }

  neopixels.show();
}

void ledsSetAnim(LedAnim anim) {
  ledAnim = anim;
  ledAnimLastAt = 0;
  ledAnimPhase = 0;
}

void ledsService() {
  if (ledAnim == LED_ANIM_OFF) return;
  if (millis() - ledAnimLastAt < 350) return;

  ledAnimLastAt = millis();
  ledAnimPhase = (ledAnimPhase + 1) & 1;

  switch (ledAnim) {
    case LED_ANIM_PRESENCE:
      pixelsSetAll(ledAnimPhase ? COLOR_DIM_RED : COLOR_DIM_BLUE);
      break;
    case LED_ANIM_ACK:
      pixelsSetAll(ledAnimPhase ? COLOR_DIM_GREEN : COLOR_DIM_WHITE);
      break;
    case LED_ANIM_DUPLICATE:
      pixelsSetAll(ledAnimPhase ? COLOR_DIM_CYAN : COLOR_DIM_MAGENTA);
      break;
    case LED_ANIM_DUMP:
      pixelsSetAll(ledAnimPhase ? COLOR_DIM_YELLOW : COLOR_DIM_WHITE);
      break;
    default:
      break;
  }
}

void drawScreen(const String &title, const String &message, uint16_t shownId, const String &footer) {
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();

  bool compact = display.height() < 120;
  int16_t titleY = compact ? 18 : 24;
  int16_t infoY = compact ? 38 : 50;
  int16_t seenY = compact ? 55 : 70;
  int16_t contentY = compact ? 62 : 80;
  int16_t messageY = compact ? 80 : 96;
  int16_t footerY = compact ? 101 : 117;
  int16_t blocksX = (display.width() - 144) / 2;
  if (blocksX < 4) blocksX = 4;

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    drawCentered(title, titleY, &FreeMonoBold12pt7b);

    display.setFont(&FreeMono9pt7b);
    display.setCursor(8, infoY);
    display.print("ID ");
    display.print(storageMyId, HEX);
    display.print("  team ");
    display.print(teamName(storageMyTeam));

    display.setCursor(8, seenY);
    display.print("seen ");
    display.print(storageSeenCount);
    display.print(" badges");

    if (shownId) {
      drawIdBlocks(shownId, blocksX, contentY);
    } else {
      drawCentered(message, messageY, &FreeMonoBold9pt7b);
    }

    display.setFont(&FreeMono9pt7b);
    display.setCursor(8, footerY);
    display.print(footer);
  } while (display.nextPage());
}

void drawHome(const String &footer) {
  drawScreen("MF 2019 game", "ready", storageMyId, footer);
}

void drawHome() {
  drawHome("T1/2 pair, T4/5 show");
}

bool bootPressed() {
  return digitalRead(PIN_BOOT_BUTTON) == LOW;
}

bool pairTouched() {
  return touchPinTouched(PIN_TOUCH_PAIR_1) || touchPinTouched(PIN_TOUCH_PAIR_2);
}

bool showTouched() {
  return touchPinTouched(PIN_TOUCH_SHOW_1) ||
         touchPinTouched(PIN_TOUCH_SHOW_2);
}
