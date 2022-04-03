#ifdef NTP
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "br.pool.ntp.org", -3 * 3600, 60000);

unsigned long EpochTime() {
	return timeClient.getEpochTime();
}

void setup_ntp() {
  timeClient.begin();
  setSyncProvider(&EpochTime);
}

void loop_ntp() {
  timeClient.update();
}

#endif