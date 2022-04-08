#ifndef LOG_HH
#define LOG_HH
#include <Arduino.h>

#define LOG(x) Log(x)

void setup_log();
void Log(String &m);
void Log(char const*m);
#endif
