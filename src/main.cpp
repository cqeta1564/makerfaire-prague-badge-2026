#include <Arduino.h>
#include <esp_system.h>

#include "BadgeConfig.h"
#include "BadgeDisplay.h"
#include "BadgeStorage.h"
#include "GameLogic.h"
#include "IrProtocol.h"
#include "SerialCommands.h"

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);

  randomSeed(uint32_t(ESP.getEfuseMac()) ^ esp_random());

  badgeDisplaySetup();
  storageSetup();
  irRecvSetup();

  drawHome("ready");

  Serial.println(F("+MFBadge2019-MakerBadge ready"));
  printHelp();
}

void loop() {
  serviceSerial();
  ledsService();
  gameLoop();
}
