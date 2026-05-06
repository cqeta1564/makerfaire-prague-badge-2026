#pragma once

#include <Arduino.h>

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

enum BadgeMode {
  MODE_IDLE,
  MODE_SHOW,
  MODE_PAIRING,
  MODE_AWAIT_CONFIRM,
  MODE_CONFIRMED,
  MODE_DUMP
};

enum LedAnim {
  LED_ANIM_OFF,
  LED_ANIM_PRESENCE,
  LED_ANIM_ACK,
  LED_ANIM_DUPLICATE,
  LED_ANIM_DUMP
};
