#pragma once

#include <Arduino.h>

constexpr uint8_t IR_CMD_FLAG = 0x30;

constexpr uint8_t IR_CMD_PRESENCE = IR_CMD_FLAG | 1;
constexpr uint8_t IR_CMD_PAIR_ACK = IR_CMD_FLAG | 2;
constexpr uint8_t IR_CMD_PAIR_CONFIRM = IR_CMD_FLAG | 3;
constexpr uint8_t IR_CMD_DUMP_REQUEST = IR_CMD_FLAG | 4;
constexpr uint8_t IR_CMD_DUMP_ACK = IR_CMD_FLAG | 5;
constexpr uint8_t IR_CMD_DUMP_INFO = IR_CMD_FLAG | 6;
constexpr uint8_t IR_CMD_PRESENCE_OLD = IR_CMD_FLAG | 7;
constexpr uint8_t IR_CMD_CONFIRM_OLD = IR_CMD_FLAG | 8;
constexpr uint8_t IR_CMD_TEAM_SET = IR_CMD_FLAG | 9;
constexpr uint8_t IR_CMD_TEAM_CONFIRM = IR_CMD_FLAG | 10;

constexpr uint8_t IR_CMD_LEGACY_ACKNOWLEDGE = 0b010101;

struct IrPacket {
  uint8_t cmd = 0;
  uint8_t teamId = 0;
  uint16_t id1 = 0;
  uint16_t id2 = 0;
  bool isDump = false;
  bool isOld = false;
  uint8_t pageId = 0;
  uint8_t data[3] = {0, 0, 0};
};

void irRecvSetup();
void irRecvPower(bool on);
void irRecvCommandsClear();
bool irCommandReceived(IrPacket &packet);
void irCommandSend(uint8_t cmd, uint8_t team, uint16_t id1, uint16_t id2);
void irCommandSendDump(uint8_t pageId, uint8_t b1, uint8_t b2, uint8_t b3);
void irCommandSendOld(uint8_t cmd, uint16_t id1, uint16_t id2);
