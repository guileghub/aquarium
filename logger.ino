void setup_log() {
  Serial.begin(115200);
  delay(1000);
}

void Log(String &m) {
  Serial.println(m);
#ifdef WEB
  broadcastLog(m);
#endif
  m.clear();
}

void Log(char const*m) {
  String log = m;
  Log(log);
}
