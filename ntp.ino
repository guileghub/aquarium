#ifdef NTP
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "br.pool.ntp.org", -3 * 3600, 60000);

void setup_ntp() {
  timeClient.begin();
#error  unsigned long now = timeClient.getEpochTime();
#error log += timeClient.getFormattedTime();
}
void loop_ntp() {
  timeClient.update();
}
#endif