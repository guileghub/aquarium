#ifdef TEMP_RECORD
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AutoPID.h>
#include <vector>

// 1 week
#define TEMP_HIST_LEN (7*24*60/5)
// @5min
#define TEMP_HIST_PERIOD 5*60

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#define ONE_WIRE_BUS D2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);


long lastTemp, lastSched; //The last measurement
const int temp_cycle = 1; // @1s
unsigned char *temps = nullptr;

#ifdef AUTOPID
double current_temperature = 0;
#define PWM_PERIOD 1000
#define KP .12
#define KI .0003
#define KD 0
double target_temperature = 0;
bool pid_enabled = false;
bool relay = false;
bool output = false;
#endif

const int sched_cycle = 60;
int sched_output[SCHED_NUM] = { -1, -1, -1};

void TempLoop(long now);
void SchedLoop(long now);
String GetAddressToString(DeviceAddress deviceAddress);
bool handleFileRead(String path);

#ifdef AUTOPID
AutoPIDRelay autopid(&current_temperature, &target_temperature, &relay, PWM_PERIOD, KP, KI, KD);
#endif

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


struct TempMeasure {
  // 0 -> -5, 1->-4.75 2-> -4.5 254->58.5 255->invalid
  unsigned char value;
  TempMeasure(): value(0xFF) {
  }
  TempMeasure(float temp) {
    if (temp > -5 && temp <= 58.5) {
      temp += 5;
      temp *= 4;
      if (temp < 0) temp = 0;
      if (temp > 254) temp = 254;
      value = static_cast<unsigned char>(temp);
    } else value = 0xFF;
  }
  operator float() const {
    if (value == 0xFF)
      return NAN;
    float res = value;
    res /= 4;
    res -= 5;
    return res;
  }
  operator bool() const {
    return value != 0xFF;
  }
};
struct TempHistory {
  std::vector<TempMeasure> hist;
  size_t time_interval;
  size_t capacity;
  size_t current;
  unsigned long lastRecordEpochTime;
  void clear() {
    current = 0, lastRecordEpochTime = 0;
    hist.clear();
    Log("clear");
  }
  TempHistory(size_t time_interval, size_t capacity): time_interval(time_interval), capacity(capacity) {
    clear();
  }
  void addRecord(TempMeasure const& t) {
    if (hist.size() <= capacity) {
      hist.push_back(t);
      current = hist.size();
    } else {
      if (current >= capacity)
        current = 0;
      hist[current] = t;
      current++;
    }
  }
  void updateRecord(TempMeasure const& t) {
    if (hist.empty())
      addRecord(t);
    else
      hist[current - 1] = t;
  }
  void record(TempMeasure const& t, unsigned long epochTime) {
    String log;
    log += "epochTime: ";
    log += epochTime;
    log += " lastRecordEpochTime:";
    log += lastRecordEpochTime;
    if (epochTime < lastRecordEpochTime)
      clear();
    if (!lastRecordEpochTime)
      lastRecordEpochTime = epochTime;
    unsigned long gap = (epochTime - lastRecordEpochTime) / time_interval;
    log += " gap:";
    log += gap;
    Log(log);
    if (gap >= capacity) {
      clear();
      gap = 0;
    }
    if (gap) {
      while (gap--)
        addRecord(TempMeasure());
      lastRecordEpochTime = epochTime;
    }
    updateRecord(t);
    log = "hist.size()=";
    log += hist.size();
    log += " current=";
    log += current;
    Log(log);
  }
  std::vector<TempMeasure> query(unsigned long begin, unsigned long end) {
    std::vector<TempMeasure> result;
    size_t size = hist.size();
    long deltaBegin = (lastRecordEpochTime - begin) / time_interval;
    if (deltaBegin >= size)
      return result;
    long deltaEnd = (lastRecordEpochTime - end) / time_interval;
    if (deltaEnd >= size)
      return result;
    size_t i, e;
    if (deltaBegin < (size - current)) {
      i = size - current + deltaBegin;
    } else {
      i = deltaBegin + current - size;
    }
    if (deltaEnd < (size - current)) {
      e = size - current + deltaEnd;
    } else {
      e = deltaEnd + current - size;
    }
    for (; i != e; i++) {
      result.push_back(hist[i]);
      if (i >= size)
        i = 0;
    }
    return result;
  }
};

struct TempDevice {
  DeviceAddress dev_addr;
  String name;
  TempHistory history;
  TempDevice() : history(TEMP_HIST_PERIOD, TEMP_HIST_LEN) {
  }
};

std::vector<TempDevice> temp_devs;

void setupWiFi() {
  /*  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("STA Failed to configure");
    }*/
  WiFi.setHostname(hostname);
  Serial.println("");
  Serial.println("WiFi connecting.");
  WiFi.begin(ssid, password);
  /*
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    } */
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

#ifdef TEMP_RECORD
//Setting the temperature sensor
void SetupDS18B20() {
  DS18B20.begin();

  String log = "Parasite power is: ";
  if (DS18B20.isParasitePowerMode()) {
    log += "ON";
  } else {
    log += "OFF";
  }
  Log(log);

  int numberOfDevices = DS18B20.getDeviceCount();
  log += "Device count: ";
  log += numberOfDevices;
  Log(log);
  if (numberOfDevices > 0) {
    temp_devs.clear();
    temp_devs.resize(numberOfDevices);
  }

  DS18B20.requestTemperatures();

  for (int i = 0; i < numberOfDevices; i++) {
    // Search the wire for address
    if (DS18B20.getAddress(temp_devs[i].dev_addr, i)) {
      temp_devs[i].name = GetAddressToString(temp_devs[i].dev_addr);
      log += "Found device ";
      log += i;
      log += "DEC";
      log += " with address: ";
      log += GetAddressToString(temp_devs[i].dev_addr);
    } else {
      temp_devs[i].name = "unknown";
      log += "Found ghost device at ";
      log += i;
      log += "DEC";
      log +=
        " but could not detect address. Check power and cabling";
    }
    log += " Resolution: ";
    log += DS18B20.getResolution(temp_devs[i].dev_addr);
    Log(log);
  }
}
#endif
/*
  void SetupPID() {
  autopid.setBangBang(4);
  autopid.setTimeStep(temp_cycle);
  }
*/
void power_control(int port, bool val) {
  char port_code;
  switch (port) {
    case 1:
      port_code = '1';
      break;
    case 2:
      port_code = '2';
      break;
    case 3:
      port_code = '3';
      break;
    default:
      port_code = '0'; // all
  }
  String cmd = "C";
  cmd += port_code;
  cmd += val ? '1' : '0';
  String log = "Sending command: ";
  log += cmd;
  Log(log);
  Serial1.print(cmd);
}

void setupOTA() {

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("aquario");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
      SPIFFS.end();
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void setup() {
  ESP.wdtEnable(WDTO_8S);
  Serial.begin(115200);
  Serial1.begin(9600);
  delay(1000);
  pinMode(OUTPUT_PIN, OUTPUT);
  SPIFFS.begin();
  Serial.println();
  Serial.print("Configuring access point...");
  setupWiFi();
  setupOTA();
  timeClient.begin();

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

  unsigned long now = timeClient.getEpochTime();
  lastTemp = lastSched = now;

  SetupDS18B20();

  TempLoop(now);
  SchedLoop(now);
#ifdef AUTOPID
  SetupPID();
#endif
  yield();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      connected = false;
      yield();
      break;

    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
                      ip[1], ip[2], ip[3], payload);
        yield();
        web_sock_number = num;
        connected = true;
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
      connected = false;
      yield();
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

String GetAddressToString(DeviceAddress deviceAddress) {
  String str = "";
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16)
      str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
}

static void fill_temps(JsonDocument &response, unsigned long begin, unsigned long end) {
  //week temp records
  JsonObject temps = response.createNestedObject("temperatures");
  size_t devs_num = temp_devs.size();
  JsonArray td[devs_num];
  for (int i = 0; i < devs_num; i++)
    td[i] = temps.createNestedArray(temp_devs[i].name);
  for (int i = 0; i < devs_num; i++) {
    std::vector<TempMeasure> tv = temp_devs[i].history.query(begin, end);
    for (auto t : tv) {
      td[i].add(static_cast<float>(t));
      if (response.memoryUsage() + 64 > response.capacity())
        return;
    }
  }
}
void temperatureUpdate(unsigned long now) {
  if (!connected)
    return;
  DynamicJsonDocument response(4096);
  fill_temps(response, now - 60, now);
  String message;
  serializeJson(response, message);
  webSocket.sendTXT(web_sock_number, message);
}
void schedUpdate(unsigned long now) {
  if (!connected)
    return;
  DynamicJsonDocument response(4096);
  JsonArray outputs = response.createNestedArray("outputs");
  for (unsigned x = 0; x < SCHED_NUM; x++) {
    outputs.add(sched_output[x]);
  }
  String message;
  serializeJson(response, message);
  webSocket.sendTXT(web_sock_number, message);
}
void send_update() {
  if (!connected)
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
  webSocket.sendTXT(web_sock_number, message);
}

void parse_message(uint8_t *payload,
                   size_t length) {
  String log;
  int numberOfDevices = temp_devs.size();
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
    for (int i = 0; i < numberOfDevices; i++) {
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

void SchedLoop(unsigned long now) {
  String log = "SchedLoop ";
  log += timeClient.getFormattedTime();
  Log(log);
  // if(timeClient.isTimeSet()){
  int hour = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int sched_new[SCHED_NUM];
  if (hour > 17 || hour < 5)
    sched_new[0] = true;
  else
    sched_new[0] = false;
  if (hour > 6 || hour < 18)
    sched_new[1] = false;
  else
    sched_new[1] = true;
  sched_new[2] = sched_output[2];

  for (unsigned x = 0; x < SCHED_NUM; x++) {
    if (sched_new[x] != sched_output[x]) {
      sched_output[x] = sched_new[x];
      power_control(x + 1, sched_output[x]);
    }
  }
  lastSched = now;
  schedUpdate(now);
}
//Loop measuring the temperature
void TempLoop(unsigned long now) {
  int numberOfDevices = temp_devs.size();
  for (int i = 0; i < numberOfDevices; i++) {
    float tempC = DS18B20.getTempC(temp_devs[i].dev_addr); //Measuring temperature in Celsius
    TempMeasure t(tempC);
    String log;
    log += "Temperature[";
    log += temp_devs[i].name;
    log += "]@(";
    log += now;
    log += ")=";
    log += tempC;
    log += "C";
    Log(log);
    temp_devs[i].history.record(t, now);
  }
  DS18B20.setWaitForConversion(false); //No waiting for measurement
  DS18B20.requestTemperatures(); //Initiate the temperature measurement
  temperatureUpdate(now);
  lastTemp = now;  //Remember the last time measurement
}

void loop() {
  ESP.wdtFeed();
  ArduinoOTA.handle();
  unsigned long now = timeClient.getEpochTime();
  if (now - lastTemp > temp_cycle)
    TempLoop(now);
  if (now - lastSched > sched_cycle)
    SchedLoop(now);

#ifdef AUTOPID
  if (pid_enabled) {
    autopid.run();
    output = relay;
  }
  digitalWrite(OUTPUT_PIN, output);
#endif

  server.handleClient();
  webSocket.loop();
  timeClient.update();
  /*
    if (timeClient.isTimeSet())
    log+="NTP SET");
    else
    log+="NTP ???");

    if (hour == timeClient.getHours() && minute == timeClient.getMinutes()) {
      digitalWrite(led, 0);
    }
    }
  */
  //Serial.println(timeClient.getFormattedTime());
  //delay(100);
}
