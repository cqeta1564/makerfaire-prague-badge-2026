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
void drawScreen(const String &title, const String &message, uint16_t shownId, const String &footer);
void drawHome(const String &footer);
void drawHome();
bool bootPressed();
bool pairTouched();
bool showTouched();
