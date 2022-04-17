#include <ostream>
#include <iomanip>
#include <ios>
#include <TimeLib.h>
#include "iso_string.hh"

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
