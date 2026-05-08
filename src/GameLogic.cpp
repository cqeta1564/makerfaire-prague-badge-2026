#include "GameLogic.h"

#include "BadgeConfig.h"
#include "BadgeDisplay.h"
#include "BadgePower.h"
#include "BadgeStorage.h"
#include "BadgeTypes.h"
#include "IrProtocol.h"

namespace {
constexpr uint8_t LOGIC_DUMP_PAGE_SIZE = 3;
constexpr uint8_t LOGIC_DUMP_MAX_PAGE = STORAGE_COUNT / LOGIC_DUMP_PAGE_SIZE + 1;
constexpr uint8_t LOGIC_DUMP_INVALID_PAGE = 0xff;

BadgeMode badgeMode = MODE_IDLE;

uint32_t modeStartedAt = 0;
uint32_t nextPresenceAt = 0;
uint32_t nextShowAt = 0;
uint32_t nextDumpAt = 0;

uint16_t logicLastShownId = 0;
uint16_t logicPairPresenceBadgeId = 0;
uint8_t logicPairPresenceSent = 0;
uint8_t pairPresenceRemaining = 0;
uint16_t showRemaining = 0;

uint8_t logicDumpPage = LOGIC_DUMP_INVALID_PAGE;
uint8_t logicDumpAttempt = 0;

void enterFinalSleep(const String &footer) {
  (void)footer;
  drawSleepScreen();
  badgePowerEnterFinalSleep();
}

uint16_t logicMyTeamId() {
  switch (storageMyTeam) {
    case STORAGE_TEAM_RED: return PIXELS_ALL_RED;
    case STORAGE_TEAM_GREEN: return PIXELS_ALL_GREEN;
    case STORAGE_TEAM_BLUE: return PIXELS_ALL_BLUE;
    default: return 0;
  }
}

uint8_t logicDumpSeenPagesCount() {
  uint8_t result = 0;
  uint8_t inPageResult = 0;
  uint8_t pageCounter = 0;

  for (uint16_t i = 0; i < STORAGE_COUNT; i++) {
    if (storageSeenIds[i]) inPageResult = 1;

    pageCounter++;
    if (pageCounter == LOGIC_DUMP_PAGE_SIZE) {
      result += inPageResult;
      inPageResult = 0;
      pageCounter = 0;
    }
  }

  if (pageCounter) result += inPageResult;
  return result;
}

uint8_t logicDumpFindNonEmptyPage(uint8_t startPage) {
  if (startPage >= LOGIC_DUMP_MAX_PAGE) return LOGIC_DUMP_INVALID_PAGE;

  for (uint16_t i = startPage * LOGIC_DUMP_PAGE_SIZE; i < STORAGE_COUNT; i++) {
    if (storageSeenIds[i]) return i / LOGIC_DUMP_PAGE_SIZE;
  }

  return LOGIC_DUMP_INVALID_PAGE;
}

void logicPairBadgeConfirmed(uint16_t id) {
  String footer;

  if (id == 0) {
    ledsSetAnim(LED_ANIM_ACK);
    footer = "team assigned";
  } else if (storageIdSeen(id)) {
    ledsSetAnim(LED_ANIM_DUPLICATE);
    footer = "already seen";
  } else {
    ledsSetAnim(LED_ANIM_ACK);
    footer = "new badge saved";
    if (id != STORAGE_WILDCARD_ID) storageMarkIdSeen(id);
  }

  uint16_t shownId = id ? id : logicMyTeamId();
  pixelsShowId(shownId);
  badgeMode = MODE_CONFIRMED;
  modeStartedAt = millis();
  (void)footer;
  drawShowScreen();
}

void logicDumpRespond(const IrPacket &packet) {
  if (!logicPairPresenceSent) return;
  if (packet.id1 != storageMyId) return;

  uint8_t seenPagesCount = logicDumpSeenPagesCount();
  irCommandSend(IR_CMD_DUMP_INFO, storageMyTeam, storageMyId, seenPagesCount);

  logicDumpPage = logicDumpFindNonEmptyPage(0);
  logicDumpAttempt = 0;

  if (seenPagesCount && logicDumpPage != LOGIC_DUMP_INVALID_PAGE) {
    badgeMode = MODE_DUMP;
    modeStartedAt = millis();
    nextDumpAt = millis();
    ledsSetAnim(LED_ANIM_DUMP);
    drawPairingScreen();
  } else {
    returnToIdle("nothing to dump");
  }
}

void handlePairPacket(const IrPacket &packet) {
  if (packet.cmd == IR_CMD_PRESENCE || packet.cmd == IR_CMD_PRESENCE_OLD) {
    bool isOld = packet.cmd == IR_CMD_PRESENCE_OLD;

    if (isOld && (packet.id1 != (~packet.id2 & 0x1ff))) return;
    if (packet.id1 == storageMyId) return;

    pairPresenceRemaining = 0;
    if (isOld) {
      irCommandSendOld(IR_CMD_LEGACY_ACKNOWLEDGE, storageMyId & 0x1ff, packet.id1);
    } else {
      irCommandSend(IR_CMD_PAIR_ACK, storageMyTeam, storageMyId, packet.id1);
    }

    logicPairPresenceBadgeId = packet.id1;
    badgeMode = MODE_AWAIT_CONFIRM;
    modeStartedAt = millis();
    // Keep the IR handshake free of full e-paper refreshes; the confirm window
    // is short and the old 2019 firmware sends its confirm immediately.
    return;
  }

  if (packet.cmd == IR_CMD_PAIR_ACK) {
    if (packet.id2 != storageMyId) return;
    if (!storageIdValid(packet.id1)) return;
    if (!logicPairPresenceSent) return;

    pairPresenceRemaining = 0;
    irCommandSend(IR_CMD_PAIR_CONFIRM, storageMyTeam, storageMyId, packet.id1);
    logicPairBadgeConfirmed(packet.id1);
    return;
  }

  if (packet.cmd == IR_CMD_TEAM_SET) {
    if (packet.id2 != storageMyId) return;
    if (!logicPairPresenceSent) return;
    if (storageMyTeam != STORAGE_TEAM_UNDECIDED) return;
    if (packet.id1 >= STORAGE_MAX_TEAM || packet.id1 == STORAGE_TEAM_UNDECIDED) return;

    pairPresenceRemaining = 0;
    storageSetTeam(packet.id1);
    irCommandSend(IR_CMD_TEAM_CONFIRM, storageMyTeam, storageMyId, packet.id1);
    logicPairBadgeConfirmed(0);
    return;
  }

  if (packet.cmd == IR_CMD_DUMP_REQUEST) {
    logicDumpRespond(packet);
  }
}

void handleConfirmPacket(const IrPacket &packet) {
  if (packet.cmd != IR_CMD_PAIR_CONFIRM && packet.cmd != IR_CMD_CONFIRM_OLD) return;

  bool isOld = packet.cmd == IR_CMD_CONFIRM_OLD;
  uint16_t myId = isOld ? (storageMyId & 0x1ff) : storageMyId;

  if (packet.id2 != myId) return;
  if (!storageIdValid(packet.id1, isOld)) return;
  if (packet.id1 != logicPairPresenceBadgeId) return;

  logicPairBadgeConfirmed(packet.id1);
}

bool showNextSeenBadge() {
  while (logicLastShownId < STORAGE_MAX_ID) {
    uint16_t id = logicLastShownId++;
    if (!storageIdSeen(id)) continue;

    showRemaining--;
    pixelsShowId(id);
    return true;
  }

  showRemaining = 0;
  return false;
}

void serviceShow() {
  if (pairTouched()) {
    delay(40);
    if (pairTouched()) startPairing();
    return;
  }

  if (millis() < nextShowAt) return;
  nextShowAt = millis() + 1200;

  if (showRemaining == 0) {
    returnToIdle("show complete");
    return;
  }

  if (!showNextSeenBadge()) returnToIdle("show complete");
}

void servicePairing() {
  if (badgeMode == MODE_PAIRING && pairPresenceRemaining && millis() >= nextPresenceAt) {
    irCommandSend(IR_CMD_PRESENCE, storageMyTeam, storageMyId, storageSeenCount);
    logicPairPresenceSent = 1;
    pairPresenceRemaining--;
    nextPresenceAt = millis() + random(1600, 2000);
  }

  IrPacket packet;
  while (irCommandReceived(packet)) {
    if (badgeMode == MODE_AWAIT_CONFIRM) {
      handleConfirmPacket(packet);
    } else {
      handlePairPacket(packet);
    }

    if (badgeMode == MODE_CONFIRMED || badgeMode == MODE_DUMP || badgeMode == MODE_IDLE) return;
  }

  if (badgeMode == MODE_PAIRING && pairPresenceRemaining == 0 && millis() - modeStartedAt > 7000) {
    returnToIdle("no badge found");
  }

  if (badgeMode == MODE_AWAIT_CONFIRM && millis() - modeStartedAt > 3000) {
    returnToIdle("pair timeout");
  }
}

void serviceDump() {
  IrPacket packet;
  while (irCommandReceived(packet)) {
    if (packet.cmd == IR_CMD_DUMP_ACK) {
      if (packet.id1 != storageMyId) continue;
      if (packet.id2 != logicDumpPage) continue;

      logicDumpPage = logicDumpFindNonEmptyPage(logicDumpPage + 1);
      logicDumpAttempt = 0;
      nextDumpAt = millis();

      if (logicDumpPage == LOGIC_DUMP_INVALID_PAGE) {
        returnToIdle("dump complete");
      }
      return;
    }
  }

  if (logicDumpPage == LOGIC_DUMP_INVALID_PAGE) {
    returnToIdle("dump complete");
    return;
  }

  if (millis() >= nextDumpAt) {
    uint16_t addr = logicDumpPage * LOGIC_DUMP_PAGE_SIZE;
    irCommandSendDump(logicDumpPage, storageGetSeenByte(addr), storageGetSeenByte(addr + 1), storageGetSeenByte(addr + 2));
    logicDumpAttempt++;
    nextDumpAt = millis() + 1000;
  }

  if (logicDumpAttempt >= 3 && millis() - modeStartedAt > 3500) {
    returnToIdle("dump timeout");
  }
}
}

void startShow() {
  badgeMode = MODE_SHOW;
  modeStartedAt = millis();
  nextShowAt = millis() + 1200;
  logicLastShownId = 0;
  showRemaining = storageSeenCount;
  pixelsShowId(storageMyId);
  drawShowScreen();
}

void startPairing() {
  badgeMode = MODE_PAIRING;
  modeStartedAt = millis();
  logicPairPresenceSent = 0;
  logicPairPresenceBadgeId = 0;
  pairPresenceRemaining = 3;
  nextPresenceAt = millis() + random(100, 500);

  irRecvCommandsClear();
  ledsSetAnim(LED_ANIM_PRESENCE);
  drawPairingScreen();
}

void returnToIdle(const String &footer) {
  badgeMode = MODE_IDLE;
  modeStartedAt = millis();
  pixelsOff();
#if SLEEP_AFTER_ACTION
  enterFinalSleep(footer);
#else
  drawHome(footer);
#endif
}

void gameLoop() {
  if (badgeMode == MODE_IDLE) {
    if (pairTouched()) {
      delay(40);
      if (pairTouched()) startPairing();
    } else if (showTouched()) {
      delay(40);
      if (showTouched()) startShow();
    } else if (IDLE_SLEEP_TIMEOUT_MS > 0 && millis() - modeStartedAt >= IDLE_SLEEP_TIMEOUT_MS) {
      enterFinalSleep("idle timeout");
    }
  } else if (badgeMode == MODE_SHOW) {
    serviceShow();
  } else if (badgeMode == MODE_PAIRING || badgeMode == MODE_AWAIT_CONFIRM) {
    servicePairing();
  } else if (badgeMode == MODE_CONFIRMED) {
    if (millis() - modeStartedAt > 3000) {
      returnToIdle("ready");
    }
  } else if (badgeMode == MODE_DUMP) {
    serviceDump();
  }
}
