#include "SerialCommands.h"

#include <Arduino.h>

#include "BadgeDisplay.h"
#include "BadgePower.h"
#include "BadgeStorage.h"
#include "GameLogic.h"

namespace {
String serialLine;

void printHexByte(uint8_t value) {
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

void printHexWord(uint16_t value) {
  if (value < 0x1000) Serial.print('0');
  if (value < 0x100) Serial.print('0');
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

bool parseHexWord(const String &text, uint8_t start, uint16_t &value) {
  if (text.length() < start + 4) return false;

  value = 0;
  for (uint8_t i = 0; i < 4; i++) {
    char ch = text[start + i];
    uint8_t nibble;
    if (ch >= '0' && ch <= '9') nibble = ch - '0';
    else if (ch >= 'a' && ch <= 'f') nibble = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F') nibble = ch - 'A' + 10;
    else return false;

    value = (value << 4) | nibble;
  }

  return true;
}

void handleCommand(String command) {
  command.trim();
  if (!command.length()) return;

  char op = command[0];
  if (op >= 'a' && op <= 'z') op -= 32;

  if (op == '?') {
    Serial.println(F("+MFBadge2019-MakerBadge/1.0"));
    return;
  }

  if (op == 'H') {
    printHelp();
    return;
  }

  if (op == 'I') {
    if (command.length() == 1) {
      Serial.print(F("+I"));
      printHexWord(storageMyId);
      Serial.println();
      return;
    }

    uint16_t id;
    if (parseHexWord(command, 1, id) && storageSetId(id)) {
      Serial.print(F("+I"));
      printHexWord(storageMyId);
      Serial.println();
      drawHome("id changed");
      return;
    }
  }

  if (op == 'T') {
    if (command.length() == 1) {
      Serial.print(F("+T"));
      Serial.println(storageMyTeam);
      return;
    }

    uint8_t team = command[1] - '0';
    if (storageSetTeam(team)) {
      Serial.print(F("+T"));
      Serial.println(storageMyTeam);
      drawHome("team changed");
      return;
    }
  }

  if (op == 'C') {
    Serial.print(F("+C"));
    printHexWord(storageSeenCount);
    Serial.println();
    return;
  }

  if (op == 'D') {
    Serial.print(F("+D"));
    for (uint16_t i = 0; i < STORAGE_COUNT; i++) {
      printHexByte(storageSeenIds[i]);
    }
    Serial.println();
    return;
  }

  if (op == 'E') {
    for (uint16_t id = 0; id < STORAGE_MAX_ID; id++) {
      if (!storageIdSeen(id)) continue;
      Serial.print('>');
      printHexWord(id);
      Serial.println();
    }
    Serial.println(F("+E"));
    return;
  }

  if (op == 'F' && command.length() > 1 && command[1] == '!') {
    storageFormat();
    Serial.println(F("+F"));
    drawHome("seen storage cleared");
    return;
  }

  if (op == 'P') {
    Serial.println(F("+P"));
    startPairing();
    return;
  }

  if (op == 'V') {
    Serial.println(F("+V"));
    startShow();
    return;
  }

  if (op == 'S') {
    Serial.println(F("+S"));
    returnToIdle("idle");
    return;
  }

  if (op == 'Z' && (command.length() == 1 || (command.length() == 2 && command[1] == '!'))) {
    Serial.println(F("+Z"));
    drawScreen("Sleeping", "reset to wake", storageMyId, "serial command");
    badgePowerEnterFinalSleep();
    return;
  }

  Serial.println(F("!E"));
}
}

void printHelp() {
  Serial.println(F("+Commands:"));
  Serial.println(F("  ?    firmware info"));
  Serial.println(F("  H    help"));
  Serial.println(F("  I    get id"));
  Serial.println(F("  Ixxxx set id, 4 hex digits"));
  Serial.println(F("  T    get team"));
  Serial.println(F("  Tn   set team 0 none, 1 red, 2 green, 3 blue"));
  Serial.println(F("  C    get seen count"));
  Serial.println(F("  D    dump raw seen bitmap"));
  Serial.println(F("  E    list seen ids"));
  Serial.println(F("  F!   format seen storage"));
  Serial.println(F("  P    start pairing"));
  Serial.println(F("  V    show ids"));
  Serial.println(F("  S    return to idle"));
  Serial.println(F("  Z/Z! deep sleep now"));
}

void serviceSerial() {
  while (Serial.available()) {
    char ch = char(Serial.read());
    if (ch == '\r' || ch == '\n') {
      handleCommand(serialLine);
      serialLine = "";
    } else if (serialLine.length() < 80) {
      serialLine += ch;
    }
  }
}
