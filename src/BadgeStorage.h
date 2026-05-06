#pragma once

#include <Arduino.h>

#include "BadgeConfig.h"

extern uint8_t storageSeenIds[STORAGE_COUNT];
extern uint16_t storageSeenCount;
extern uint16_t storageMyId;
extern uint8_t storageMyTeam;

void storageSetup();
void storageFormat();
uint16_t storageCalcSeenCount();
uint8_t storageGetSeenByte(uint16_t addr);
bool storageIdValid(uint16_t id);
bool storageIdValid(uint16_t id, bool isOld);
bool storageIdSeen(uint16_t id);
void storageMarkIdSeen(uint16_t id);
bool storageSetTeam(uint8_t team);
bool storageSetId(uint16_t id);
