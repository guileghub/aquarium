#ifdef ARDUINO_ARCH_ESP32

#include <esp_task_wdt.h>

void setup_watchdog() {
  esp_task_wdt_init(8, true);
  esp_task_wdt_add(nullptr);
}
void loop_watchdog() {
  esp_task_wdt_reset();
}

#elif defined(ARDUINO_ARCH_ESP8266)

void setup_watchdog() {
  ESP.wdtEnable(WDTO_8S);
}
void loop_watchdog() {
  ESP.wdtFeed();
}

#endif
