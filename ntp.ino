#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, 0 * 3600, 60000 * 10); // update every 10 min

#ifdef ARDUINO_ARCH_ESP8266
#define EPOCH_TYPE long long
#elif defined(ARDUINO_ARCH_ESP32)
#define EPOCH_TYPE long
#endif

EPOCH_TYPE EpochTime() {
  return timeClient.getEpochTime();
}

void setup_ntp() {
  timeClient.begin();
  timeClient.forceUpdate();
  setSyncProvider(&EpochTime);
}

bool loop_ntp() {
  return timeClient.update();
}
