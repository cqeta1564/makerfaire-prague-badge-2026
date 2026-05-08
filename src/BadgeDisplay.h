#pragma once

#include <Arduino.h>

#include "BadgeTypes.h"

void badgeDisplaySetup();
void badgeDisplayPowerDown();
void pixelsOff();
void pixelsSetAll(Rgb color);
void pixelsShowId(uint16_t id);
void ledsSetAnim(LedAnim anim);
void ledsService();
void drawWakeupScreen();
void drawPairingScreen();
void drawShowScreen();
void drawSleepScreen();
void drawHome(const String &footer);
void drawHome();
bool bootPressed();
bool pairTouched();
bool showTouched();
