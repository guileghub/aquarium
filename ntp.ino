#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ostream>
#include <iomanip>
#include <ios>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "br.pool.ntp.org", 0 * 3600, 60000 * 10); // update every 10 min

long long EpochTime() {
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

String toISOString(time_t t) {
  using namespace std;
  ostringstream o;
  o << setfill('0')
    << setw(4) << year(t) << '-'
    << setw(2) << month(t) << '-'
    << setw(2) << day(t) << 'T'
    << setw(2) << hour(t) << ':'
    << setw(2) << minute(t) << ':'
    << setw(2) << second(t) << 'Z';
  return String(o.str().c_str());
}
