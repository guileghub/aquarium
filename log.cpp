#include "log.hh"
#include <ArduinoJson.h>

void broadcast_message(String &m);

void setup_log() {
  Serial.begin(115200);
  delay(100);
}

void Log(String &m) {
  Serial.println(m);
  DynamicJsonDocument log(1024);
  log["log"] = m;
  m.clear();
  serializeJson(log, m);
  broadcast_message(m);
  m.clear();
}

void Log(char const*m) {
  String log = m;
  Log(log);
}
