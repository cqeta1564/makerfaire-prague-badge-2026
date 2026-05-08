#include "IrProtocol.h"

#include "BadgeConfig.h"
#include "esp32-hal-ledc.h"

namespace {
constexpr uint8_t IR_CMD_LEGACY_PRESENCE = 0b001010;
constexpr uint8_t IR_CMD_LEGACY_CONFIRM = 0b010010;

constexpr uint8_t IR_RECV_MAX_CMDS = 8;
constexpr uint8_t IR_RECV_CMDS_MASK = 0b111;

constexpr uint8_t IR_STATE_DISABLED = 0;
constexpr uint8_t IR_STATE_H0 = 1;
constexpr uint8_t IR_STATE_D1 = 2;
constexpr uint8_t IR_STATE_D0 = 3;
constexpr uint8_t IR_STATE_INVALID = 4;
constexpr uint8_t IR_STATE_H0_OLD = 5;
constexpr uint8_t IR_STATE_D1_OLD = 6;
constexpr uint8_t IR_STATE_D0_OLD = 7;

constexpr uint16_t IR_SLOT_TIME = 282;
constexpr uint16_t IR_SLOT_TIME_OLD = 564;

constexpr uint8_t IR_MSG_LEN = 5;
constexpr uint8_t IR_MSG_OLD_LEN = 4;
constexpr uint8_t IR_RECV_SLOT_MIN = (IR_SLOT_TIME >> 6) - 1;
constexpr uint8_t IR_RECV_SLOT_MAX = (IR_SLOT_TIME >> 6) + 3;
constexpr uint8_t IR_RECV_SLOT_MIN_OLD = (IR_SLOT_TIME_OLD >> 6) - 2;
constexpr uint8_t IR_RECV_SLOT_MAX_OLD = (IR_SLOT_TIME_OLD >> 6) + 6;

constexpr uint8_t IR_RECV_CMD_INVALID = 0;
constexpr uint8_t IR_RECV_CMD_NEW = 1;
constexpr uint8_t IR_RECV_CMD_OLD = 2;

constexpr uint16_t IR_CARRIER_HZ = 38000;
constexpr uint8_t IR_TX_LEDC_CHANNEL = 0;
constexpr uint8_t IR_TX_LEDC_RESOLUTION_BITS = 8;
constexpr uint32_t IR_TX_LEDC_DUTY = (1UL << IR_TX_LEDC_RESOLUTION_BITS) / 3;

volatile uint32_t irRecvLastTime = 0;
volatile uint8_t irRecvState = IR_STATE_DISABLED;
volatile uint8_t irRecvIsrCount = 0;
bool irTxCarrierReady = false;

uint8_t irRecvIsrData[IR_MSG_LEN];
uint8_t irRecvCommands[IR_RECV_MAX_CMDS][IR_MSG_LEN + 1];
volatile uint8_t irRecvCommandsEnd = 0;
volatile uint8_t irRecvCommandsBegin = 0;

const uint8_t dscrc2x16Table[] = {
  0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
  0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
  0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
  0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74
};

uint8_t crc8(const uint8_t *addr, uint8_t len) {
  uint8_t crc = 0;

  while (len--) {
    crc = *addr++ ^ crc;
    crc = dscrc2x16Table[crc & 0x0f] ^ dscrc2x16Table[16 + ((crc >> 4) & 0x0f)];
  }

  return crc;
}

void IRAM_ATTR irRecvIsr() {
  if (irRecvState == IR_STATE_DISABLED) return;

  uint32_t t = micros();
  uint32_t t1 = (t - irRecvLastTime) >> 6;
  irRecvLastTime = t;

  if (t1 > 255) {
    irRecvState = IR_STATE_INVALID;
    return;
  }

  uint8_t delta = uint8_t(t1);
  bool levelHigh = digitalRead(PIN_IR_RX);

  if (levelHigh) {
    if (delta >= IR_RECV_SLOT_MIN * 8 && delta <= IR_RECV_SLOT_MAX * 8) {
      irRecvState = IR_STATE_H0;
    } else if (delta >= IR_RECV_SLOT_MIN_OLD * 16 && delta <= IR_RECV_SLOT_MAX_OLD * 16) {
      irRecvState = IR_STATE_H0_OLD;
    } else if ((irRecvState == IR_STATE_D1 && delta >= IR_RECV_SLOT_MIN && delta <= IR_RECV_SLOT_MAX) ||
               (irRecvState == IR_STATE_D1_OLD && delta >= IR_RECV_SLOT_MIN_OLD && delta <= IR_RECV_SLOT_MAX_OLD)) {
      bool isOld = irRecvState == IR_STATE_D1_OLD;

      if (irRecvIsrCount == 0) {
        irRecvState = IR_STATE_INVALID;
        uint8_t next = irRecvCommandsBegin;
        irRecvCommands[next][0] = isOld ? 1 : 0;
        memcpy(&(irRecvCommands[next][1]), irRecvIsrData, IR_MSG_LEN);
        next++;
        next &= IR_RECV_CMDS_MASK;
        irRecvCommandsBegin = next;
        if (irRecvCommandsBegin == irRecvCommandsEnd) {
          irRecvCommandsEnd++;
          irRecvCommandsEnd &= IR_RECV_CMDS_MASK;
        }
      } else {
        irRecvState = isOld ? IR_STATE_D0_OLD : IR_STATE_D0;
        irRecvIsrCount--;
      }
    } else {
      irRecvState = IR_STATE_INVALID;
    }
  } else {
    if (irRecvState == IR_STATE_H0 && delta >= IR_RECV_SLOT_MIN * 4 && delta <= IR_RECV_SLOT_MAX * 4) {
      irRecvState = IR_STATE_D1;
      memset(irRecvIsrData, 0, sizeof(irRecvIsrData));
      irRecvIsrCount = IR_MSG_LEN * 8;
    } else if (irRecvState == IR_STATE_H0_OLD && delta >= IR_RECV_SLOT_MIN_OLD * 8 && delta <= IR_RECV_SLOT_MAX_OLD * 8) {
      irRecvState = IR_STATE_D1_OLD;
      memset(irRecvIsrData, 0, sizeof(irRecvIsrData));
      irRecvIsrCount = IR_MSG_OLD_LEN * 8;
    } else if (irRecvState == IR_STATE_D0 && delta >= IR_RECV_SLOT_MIN * 3 && delta <= IR_RECV_SLOT_MAX * 3) {
      irRecvState = IR_STATE_D1;
      irRecvIsrData[irRecvIsrCount >> 3] |= (1 << (irRecvIsrCount & 7));
    } else if (irRecvState == IR_STATE_D0 && delta >= IR_RECV_SLOT_MIN && delta <= IR_RECV_SLOT_MAX) {
      irRecvState = IR_STATE_D1;
    } else if (irRecvState == IR_STATE_D0_OLD && delta >= IR_RECV_SLOT_MIN_OLD * 3 && delta <= IR_RECV_SLOT_MAX_OLD * 3) {
      irRecvState = IR_STATE_D1_OLD;
      irRecvIsrData[irRecvIsrCount >> 3] |= (1 << (irRecvIsrCount & 7));
    } else if (irRecvState == IR_STATE_D0_OLD && delta >= IR_RECV_SLOT_MIN_OLD && delta <= IR_RECV_SLOT_MAX_OLD) {
      irRecvState = IR_STATE_D1_OLD;
    } else {
      irRecvState = IR_STATE_INVALID;
    }
  }
}

void irRecvEnable(bool enabled) {
  irRecvState = enabled ? IR_STATE_INVALID : IR_STATE_DISABLED;
}

void irRecvPowerRail(bool on) {
#if PIN_IR_RX_POWER >= 0
  pinMode(PIN_IR_RX_POWER, OUTPUT);
  digitalWrite(PIN_IR_RX_POWER, IR_RX_POWER_ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW));
#else
  (void)on;
#endif
}

uint8_t irRecvGetCommand(uint8_t *raw) {
  uint8_t result = IR_RECV_CMD_INVALID;

  noInterrupts();
  if (irRecvCommandsBegin != irRecvCommandsEnd) {
    result = irRecvCommands[irRecvCommandsEnd][0] ? IR_RECV_CMD_OLD : IR_RECV_CMD_NEW;
    memcpy(raw, &(irRecvCommands[irRecvCommandsEnd][1]), IR_MSG_LEN);
    irRecvCommandsEnd++;
    irRecvCommandsEnd &= IR_RECV_CMDS_MASK;
  }
  interrupts();

  return result;
}

void irDecodeNew(const uint8_t *raw, IrPacket &packet) {
  packet.cmd = (raw[0] >> 2) & 0x3f;
  packet.teamId = raw[0] & 0x03;
  packet.id1 = uint16_t(raw[1]) | (uint16_t(raw[2] & 0x0f) << 8);
  packet.id2 = uint16_t(raw[2] >> 4) | (uint16_t(raw[3]) << 4);

  packet.isDump = (packet.cmd & IR_CMD_FLAG) != IR_CMD_FLAG;
  if (packet.isDump) {
    packet.cmd = 0;
    packet.pageId = raw[0];
    packet.data[0] = raw[1];
    packet.data[1] = raw[2];
    packet.data[2] = raw[3];
  }
}

void irEncodeNew(uint8_t *raw, uint8_t cmd, uint8_t team, uint16_t id1, uint16_t id2) {
  raw[0] = (team & 0x03) | ((cmd & 0x3f) << 2);
  raw[1] = id1 & 0xff;
  raw[2] = ((id1 >> 8) & 0x0f) | ((id2 & 0x0f) << 4);
  raw[3] = (id2 >> 4) & 0xff;
  raw[4] = crc8(raw, IR_MSG_LEN - 1);
}

void irDecodeOld(const uint8_t *raw, IrPacket &packet) {
  uint8_t oldCmd = raw[0] & 0x3f;
  uint16_t oldId1 = uint16_t(raw[0] >> 6) | (uint16_t(raw[1] & 0x7f) << 2);
  uint16_t oldId2 = uint16_t(raw[1] >> 7) | (uint16_t(raw[2]) << 1);

  if (oldCmd == IR_CMD_LEGACY_PRESENCE) {
    packet.cmd = IR_CMD_PRESENCE_OLD;
  } else if (oldCmd == IR_CMD_LEGACY_CONFIRM) {
    packet.cmd = IR_CMD_CONFIRM_OLD;
  } else {
    packet.cmd = 0;
  }

  packet.id1 = oldId1;
  packet.id2 = oldId2;
  packet.teamId = 0;
  packet.isOld = true;
}

void irEncodeOld(uint8_t *raw, uint8_t cmd, uint16_t id1, uint16_t id2) {
  raw[0] = (cmd & 0x3f) | ((id1 & 0x03) << 6);
  raw[1] = ((id1 >> 2) & 0x7f) | ((id2 & 0x01) << 7);
  raw[2] = (id2 >> 1) & 0xff;
  raw[3] = crc8(raw, IR_MSG_OLD_LEN - 1);
}

void irCarrierSetup() {
  if (irTxCarrierReady) return;

  pinMode(PIN_IR_TX, OUTPUT);
  digitalWrite(PIN_IR_TX, LOW);
  ledcSetup(IR_TX_LEDC_CHANNEL, IR_CARRIER_HZ, IR_TX_LEDC_RESOLUTION_BITS);
  ledcAttachPin(PIN_IR_TX, IR_TX_LEDC_CHANNEL);
  ledcWrite(IR_TX_LEDC_CHANNEL, 0);
  irTxCarrierReady = true;
}

void irCarrierOn() {
  irCarrierSetup();
  ledcWrite(IR_TX_LEDC_CHANNEL, IR_TX_LEDC_DUTY);
}

void irCarrierOff() {
  if (irTxCarrierReady) ledcWrite(IR_TX_LEDC_CHANNEL, 0);
}

void irMark(uint32_t durationUs) {
  irCarrierOn();
  delayMicroseconds(durationUs);
  irCarrierOff();
}

void irSpace(uint32_t durationUs) {
  irCarrierOff();
  delayMicroseconds(durationUs);
}

void irSendData(const uint8_t *data, uint8_t size, bool newFormat) {
  irRecvEnable(false);

  irCarrierSetup();

  irMark(newFormat ? IR_SLOT_TIME * 8 : IR_SLOT_TIME_OLD * 16);
  irSpace(newFormat ? IR_SLOT_TIME * 4 : IR_SLOT_TIME_OLD * 8);

  uint8_t i = size << 3;
  while (i--) {
    irMark(newFormat ? IR_SLOT_TIME : IR_SLOT_TIME_OLD);
    if (data[i >> 3] & (1 << (i & 7))) {
      irSpace(newFormat ? IR_SLOT_TIME * 3 : IR_SLOT_TIME_OLD * 3);
    } else {
      irSpace(newFormat ? IR_SLOT_TIME : IR_SLOT_TIME_OLD);
    }
  }

  irMark(newFormat ? IR_SLOT_TIME : IR_SLOT_TIME_OLD);
  irCarrierOff();

  irRecvEnable(true);
}
}

void irRecvCommandsClear() {
  noInterrupts();
  irRecvCommandsEnd = irRecvCommandsBegin;
  irRecvState = IR_STATE_INVALID;
  interrupts();
}

void irRecvPower(bool on) {
  if (on) {
    pinMode(PIN_IR_RX, INPUT_PULLUP);
    irRecvPowerRail(true);

    delay(10);
    irRecvState = IR_STATE_INVALID;
    irRecvLastTime = micros();
    attachInterrupt(digitalPinToInterrupt(PIN_IR_RX), irRecvIsr, CHANGE);
  } else {
    detachInterrupt(digitalPinToInterrupt(PIN_IR_RX));
    irRecvEnable(false);
    irRecvPowerRail(false);

    pinMode(PIN_IR_RX, INPUT);
  }
}

void irRecvSetup() {
  pinMode(PIN_IR_RX, INPUT_PULLUP);
  memset(irRecvCommands, 0, sizeof(irRecvCommands));
  irRecvCommandsEnd = 0;
  irRecvCommandsBegin = 0;
  irRecvPower(true);
}

bool irCommandReceived(IrPacket &packet) {
  uint8_t raw[IR_MSG_LEN] = {0, 0, 0, 0, 0};
  uint8_t type = irRecvGetCommand(raw);

  if (type == IR_RECV_CMD_OLD) {
    if (raw[IR_MSG_OLD_LEN - 1] != crc8(raw, IR_MSG_OLD_LEN - 1)) return false;
    packet = IrPacket();
    irDecodeOld(raw, packet);
    return packet.cmd != 0;
  }

  if (type == IR_RECV_CMD_NEW) {
    if (raw[IR_MSG_LEN - 1] != crc8(raw, IR_MSG_LEN - 1)) return false;
    packet = IrPacket();
    irDecodeNew(raw, packet);
    return true;
  }

  return false;
}

void irCommandSend(uint8_t cmd, uint8_t team, uint16_t id1, uint16_t id2) {
  uint8_t raw[IR_MSG_LEN];
  irEncodeNew(raw, cmd, team, id1, id2);
  irSendData(raw, IR_MSG_LEN, true);
}

void irCommandSendDump(uint8_t pageId, uint8_t b1, uint8_t b2, uint8_t b3) {
  uint8_t raw[IR_MSG_LEN];
  raw[0] = pageId;
  raw[1] = b1;
  raw[2] = b2;
  raw[3] = b3;
  raw[4] = crc8(raw, IR_MSG_LEN - 1);
  irSendData(raw, IR_MSG_LEN, true);
}

void irCommandSendOld(uint8_t cmd, uint16_t id1, uint16_t id2) {
  uint8_t raw[IR_MSG_OLD_LEN];
  irEncodeOld(raw, cmd, id1, id2);
  irSendData(raw, IR_MSG_OLD_LEN, false);
}
