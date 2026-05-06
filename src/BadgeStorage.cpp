#include "BadgeStorage.h"

#include <Preferences.h>
#include <esp_system.h>

namespace {
Preferences prefs;

uint16_t generateDefaultId() {
  uint64_t mac = ESP.getEfuseMac();
  uint32_t raw = uint32_t(mac) ^ uint32_t(mac >> 32) ^ uint32_t(esp_random());
  uint16_t id = 0;

  for (uint8_t group = 0; group < 4; group++) {
    uint8_t code = ((raw >> (group * 5)) % 7) + 1;
    id |= uint16_t(code) << (group * 3);
  }

  return id;
}
}

uint8_t storageSeenIds[STORAGE_COUNT];
uint16_t storageSeenCount = 0;
uint16_t storageMyId = 0;
uint8_t storageMyTeam = STORAGE_TEAM_UNDECIDED;

uint16_t storageCalcSeenCount() {
  uint16_t result = 0;
  for (uint16_t i = 0; i < STORAGE_COUNT; i++) {
    uint8_t b = storageSeenIds[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (b & (1 << j)) result++;
    }
  }
  return result;
}

uint8_t storageGetSeenByte(uint16_t addr) {
  return addr < STORAGE_COUNT ? storageSeenIds[addr] : 0;
}

bool storageIdValid(uint16_t id) {
  return storageIdValid(id, false);
}

bool storageIdValid(uint16_t id, bool isOld) {
  if (isOld) {
    return ((id & 0b111) != 0) &&
           ((id & 0b111000) != 0) &&
           ((id & 0b111000000) != 0) &&
           (id < 512);
  }

  return ((id & 0b111) != 0) &&
         ((id & 0b111000) != 0) &&
         ((id & 0b111000000) != 0) &&
         ((id & 0b111000000000) != 0) &&
         (id < STORAGE_MAX_ID);
}

bool storageIdSeen(uint16_t id) {
  if (id >= STORAGE_MAX_ID) return false;
  return (storageSeenIds[id >> 3] & (1 << (id & 0b111))) != 0;
}

bool storageSetTeam(uint8_t team) {
  if (team >= STORAGE_MAX_TEAM) return false;

  storageMyTeam = team;
  prefs.putUChar("team", storageMyTeam);
  return true;
}

bool storageSetId(uint16_t id) {
  if (!storageIdValid(id)) return false;

  storageMyId = id;
  prefs.putUShort("myId", storageMyId);
  return true;
}

void storageFormat() {
  memset(storageSeenIds, 0, sizeof(storageSeenIds));
  prefs.putBytes("seen", storageSeenIds, STORAGE_COUNT);
  prefs.putUChar("magic1", STORAGE_MAGIC);
  prefs.putUChar("magic2", STORAGE_MAGIC2);
  storageSeenCount = 0;
}

void storageMarkIdSeen(uint16_t id) {
  if (id >= STORAGE_MAX_ID) return;

  uint16_t offset = id >> 3;
  uint8_t oldValue = storageSeenIds[offset];
  storageSeenIds[offset] |= (1 << (id & 0b111));

  if (oldValue != storageSeenIds[offset]) {
    prefs.putBytes("seen", storageSeenIds, STORAGE_COUNT);
    storageSeenCount = storageCalcSeenCount();
  }
}

void storageSetup() {
  prefs.begin("mf2019", false);

  storageMyId = prefs.getUShort("myId", 0);
  if (!storageIdValid(storageMyId)) {
    storageSetId(generateDefaultId());
  }

  storageMyTeam = prefs.getUChar("team", STORAGE_TEAM_UNDECIDED);
  if (storageMyTeam >= STORAGE_MAX_TEAM) {
    storageSetTeam(STORAGE_TEAM_UNDECIDED);
  }

  uint8_t magic = prefs.getUChar("magic1", 0);
  uint8_t magic2 = prefs.getUChar("magic2", 0);
  if (magic == STORAGE_MAGIC && magic2 == STORAGE_MAGIC2) {
    size_t loaded = prefs.getBytes("seen", storageSeenIds, STORAGE_COUNT);
    if (loaded != STORAGE_COUNT) storageFormat();
  } else {
    storageFormat();
  }

  storageSeenCount = storageCalcSeenCount();
}
