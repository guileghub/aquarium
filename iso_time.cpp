#include <ostream>
#include <sstream>
#include <iomanip>
#include <ios>
#include <TimeLib.h>
#include "iso_time.hh"

String time_t_2_iso(time_t t) {
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

time_t iso_2_time_t(char const *iso) {
  struct tm tm = {0};
  if (nullptr == strptime(iso, "%Y-%m-%dT%H:%M:%SZ", &tm))
    return 0;
  return mktime(&tm);
}
