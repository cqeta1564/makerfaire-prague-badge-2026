#include "BadgePower.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

#include "BadgeConfig.h"
#include "BadgeDisplay.h"
#include "IrProtocol.h"

namespace {
void holdPin(gpio_num_t pin) {
  gpio_hold_en(pin);
}
}

void badgePowerEnterFinalSleep() {
  Serial.println(F("+SLEEP reset-only"));
  Serial.flush();

  badgeDisplayPowerDown();
  irRecvPower(false);

  digitalWrite(PIN_IR_TX, LOW);
  pinMode(PIN_IR_TX, INPUT);
  pinMode(PIN_IR_RX, INPUT);
  pinMode(PIN_BOOT_BUTTON, INPUT);
  pinMode(PIN_TOUCH_PAIR_1, INPUT);
  pinMode(PIN_TOUCH_PAIR_2, INPUT);
  pinMode(PIN_TOUCH_UNUSED_3, INPUT);
  pinMode(PIN_TOUCH_SHOW_1, INPUT);
  pinMode(PIN_TOUCH_SHOW_2, INPUT);

#if PIN_IR_RX_POWER >= 0
  pinMode(PIN_IR_RX_POWER, OUTPUT);
  digitalWrite(PIN_IR_RX_POWER, IR_RX_POWER_ACTIVE_LOW ? HIGH : LOW);
  holdPin(static_cast<gpio_num_t>(PIN_IR_RX_POWER));
#endif

  holdPin(static_cast<gpio_num_t>(PIN_EPD_POWER));
  holdPin(static_cast<gpio_num_t>(PIN_NEOPIXEL_PWR));
  gpio_deep_sleep_hold_en();

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

  Serial.end();
  esp_deep_sleep_start();

  while (true) {
    delay(1000);
  }
}
