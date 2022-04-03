void Log(String &m) {
  Serial.println(m);
  if (!connected) {
    m.clear();
    return;
  }
  DynamicJsonDocument log(1024);
  log["log"] = m;
  m.clear();
  serializeJson(log, m);
  webSocket.sendTXT(web_sock_number, m);
  m.clear();
}

void Log(char const*m) {
  String log = m;
  Log(log);
}
