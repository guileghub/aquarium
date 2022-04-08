#include <FS.h>
#include <WebSockets.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <set>

std::set<uint8_t> web_sock_clients;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

void setup_WEB() {
  SPIFFS.begin();
  server.on("/", HTTP_GET, []() {
    handleFileRead("/");
  });

  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.begin();
  Serial.println("HTTP server started");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      web_sock_clients.erase(num);
      yield();
      break;

    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
                      ip[1], ip[2], ip[3], payload);
        yield();
        web_sock_clients.insert(num);
        send_update();
        break;
      }

    case WStype_TEXT:
      {
        Serial.printf("[%u] get Text: %s [%d]\n", num, payload, length);
        //String message((char*)payload);
        parse_message(payload, length);
        yield();
      }
      break;

    case WStype_ERROR:
      Serial.printf("Error [%u] , %s\n", num, payload);
      web_sock_clients.erase(num);
      yield();
  }
  for (auto i = web_sock_clients.begin(), e = web_sock_clients.begin(); i != e;) {
    auto it = i++;
    if (!webSocket.clientIsConnected(*it))
      web_sock_clients.erase(*it);
  }
}

String getContentType(String filename) {
  yield();
  if (server.hasArg("download"))
    return "application/octet-stream";
  else if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  else if (filename.endsWith(".svg"))
    return "image/svg+xml";
  return "text/plain";
}

bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);

  if (path.endsWith("/")) {
    path += "index.html";
  }

  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  Serial.println("PathFile: " + pathWithGz);

  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    Serial.println("Sending: " + path);
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  String log = "File not found: ";
  log += path;
  Log(log);
  yield();
  return false;
}

void send_update() {
  if (webSocket.connectedClients() <= 0)
    return;
  DynamicJsonDocument status(4096);
#ifdef AUTOPID
  if (pid_enabled) {
    status["targetTemperature"] = target_temperature;
  }
  status["power"] = !!output;
#endif
  String message;
  serializeJson(status, message);
  webSocket.broadcastTXT(message);
}

void parse_message(uint8_t *payload,
                   size_t length) {
  String log;
  StaticJsonDocument<1024> json;
  DeserializationError error = deserializeJson(json, payload, length);
  if (error) {
    log = "deserializeJson() failed: ";
    log += error.f_str();
    Log(log);
    return;
  }
  JsonObject obj = json.as<JsonObject>();
  if (obj.isNull()) {
    Log("Error, JSON object expected.");
    return;
  }
  JsonVariant reboot = obj.getMember("reboot");
  if (reboot.is<bool>()) {
    if (reboot.as<bool>())
      ESP.restart();
  }
#ifdef AUTOPID
  JsonVariant targetTemperature = obj.getMember("targetTemperature");
  if (targetTemperature.is<double>()) {
    target_temperature = targetTemperature.as<double>();
    pid_enabled = true;
    log = String("targetTemperature=> pid_enabled=") + pid_enabled + ", output=" + output;
    Log(log);
    return;
  }
  JsonVariant power = obj.getMember("power");
  if (power.is<bool>()) {
    pid_enabled = false;
    output = (bool) power;
    log = String("POWER=> pid_enabled=") + pid_enabled + ", output=" + output;
    Log(log);
    return;
  }
#endif
  JsonVariant outputs = obj.getMember("outputs");
  if (outputs.is<JsonArray>()) {
    for (int i = 0; i < SCHED_NUM; i++) {
      JsonVariant output = outputs.getElement(i);
      if (!output.isNull()) {
        bool v = output.as<bool>();
        sched_output[i] = v;
        power_control(i + 1, sched_output[i]);
      }
    }
    return;
  }
  Log("unknown command");
}

void loop_WEB() {
  server.handleClient();
  webSocket.loop();
}

void broadcastLog(String &m) {
  if (webSocket.connectedClients() <= 0)
    return;
  DynamicJsonDocument log(1024);
  log["log"] = m;
  m.clear();
  serializeJson(log, m);
  webSocket.broadcastTXT(m);
}
