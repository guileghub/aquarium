#include <OneWire.h>
#include <DallasTemperature.h>
#include <AutoPID.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define SCHED_NUM 3
const char* ssid = "Canopus";
const char* password = "B@r@lh@d@";
const char* hostname = "aquario";
IPAddress local_IP = INADDR_NONE; //(192, 168, 15, 254);
IPAddress gateway = INADDR_NONE; //(192, 168, 15, 1);
IPAddress subnet = INADDR_NONE; //(255, 0, 0, 0);
IPAddress primaryDNS = INADDR_NONE; //(192, 168, 15, 1);   //optional
IPAddress secondaryDNS = INADDR_NONE; //(8, 8, 4, 4); //optional
uint8_t web_sock_number = 0;
bool connected = false;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "br.pool.ntp.org", -3 * 3600, 60000);
// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#define ONE_WIRE_BUS D2
#define OUTPUT_PIN D5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
int numberOfDevices;
DeviceAddress *devAddr = 0;
unsigned maxTemps = 7200;
unsigned numTemps = 0;
struct Temp {
  unsigned char duracao;
  unsigned char* medidas;
};
Temp *temps = 0;
long lastTemp, lastSched; //The last measurement
const int temp_cycle = 1000;
const int sched_cycle = 60 * 1000;

#define PWM_PERIOD 1000
#define KP .12
#define KI .0003
#define KD 0

double current_temperature = 0, target_temperature = 0;
bool pid_enabled = false;
bool relay = false;
bool output = false;
int sched_output[SCHED_NUM] = { -1, -1, -1};

void TempLoop(long now);
void SchedLoop(long now);
String GetAddressToString(DeviceAddress deviceAddress);
bool handleFileRead(String path);

AutoPIDRelay autopid(&current_temperature, &target_temperature, &relay, PWM_PERIOD, KP, KI, KD);

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

void setupWiFi() {
  /*  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("STA Failed to configure");
    }*/
  WiFi.setHostname(hostname);
  Serial.println("");
  Serial.println("WiFi connecting.");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

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

  numberOfDevices = DS18B20.getDeviceCount();
  log += "Device count: ";
  log += numberOfDevices;
  Log(log);
  if (numberOfDevices > 0)
    devAddr = new DeviceAddress[numberOfDevices];

  DS18B20.requestTemperatures();

  for (int i = 0; i < numberOfDevices; i++) {
    // Search the wire for address
    if (DS18B20.getAddress(devAddr[i], i)) {
      log += "Found device ";
      log += i;
      log += "DEC";
      log += " with address: ";
      log += GetAddressToString(devAddr[i]);
    } else {
      log += "Found ghost device at ";
      log += i;
      log += "DEC";
      log +=
        " but could not detect address. Check power and cabling";
    }
    log += " Resolution: ";
    log += DS18B20.getResolution(devAddr[i]);
    Log(log);
  }
}

void SetupPID() {
  autopid.setBangBang(4);
  autopid.setTimeStep(temp_cycle);
}

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

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  delay(1000);
  pinMode(OUTPUT_PIN, OUTPUT);
  SPIFFS.begin();
  Serial.println();
  Serial.print("Configuring access point...");
  setupWiFi();
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

  unsigned long now = millis();
  lastTemp = lastSched = now;

  SetupDS18B20();

  TempLoop(now);
  SchedLoop(now);

  SetupPID();
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

void send_update() {
  if (!connected)
    return;
  String message = "{ \"currentTemperature\" :";
  message += current_temperature;
  if (pid_enabled) {
    message += ", \"targetTemperature\" :";
    message += target_temperature;
  }
  message += ", \"power\" :";
  message += output ? "true" : "false";
  message += ", \"outputs\" : [";
  for (unsigned x = 0; x < SCHED_NUM; x++) {
    if (x)
      message += ',';
    message += sched_output[x];
  }
  message += ']';
  message += "}";
  webSocket.sendTXT(web_sock_number, message);
}

void parse_message(uint8_t *payload,
                   size_t length) {

  String message((char*)payload);
  StaticJsonDocument<1024> json;
  String log;
  DeserializationError error = deserializeJson(json, message);
  Log(message);
  log+="deserialized: ";
  serializeJson(json, log);
  Log(log);

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
  JsonVariant targetTemperature = obj.getMember("targetTemperature");
  if(targetTemperature.isNull())
    Log("targetTemperature.isNull");
  if (targetTemperature.is<double>()) {
    target_temperature = targetTemperature.as<double>();
    pid_enabled = true;
    log = String("targetTemperature=> pid_enabled=") + pid_enabled + ", output=" + output;
    Log(log);
    return;
  }
  JsonVariant power = obj.getMember("power");
  if(power.isNull())
    Log("power.isNull()");
  if (power.is<bool>()) {
    pid_enabled = false;
    output = (bool) power;
    log = String("POWER=> pid_enabled=") + pid_enabled + ", output=" + output;
    Log(log);
    return;
  }
  Log("unknown command");
  serializeJson(json, log);
  Log(log);

}

void SchedLoop(long now) {
  String log = "SchedLoop ";
  log += timeClient.getFormattedTime();
  Log(log);
  // if(timeClient.isTimeSet()){
  int hour = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  //int seconds = timeClient.getSeconds();
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
  // }
  lastSched = now;
}
//Loop measuring the temperature
void TempLoop(long now) {
  for (int i = 0; i < numberOfDevices; i++) {
    float tempC = DS18B20.getTempC(devAddr[i]); //Measuring temperature in Celsius
    //tempDev[i] = tempC; //Save the measured value to the array
    if (i == 0) {
      current_temperature = tempC;
      send_update();
    }
  }
  DS18B20.setWaitForConversion(false); //No waiting for measurement
  DS18B20.requestTemperatures(); //Initiate the temperature measurement
  lastTemp = now;  //Remember the last time measurement
}

void loop() {
  unsigned long now = millis();
  if (now - lastTemp > temp_cycle)
    TempLoop(now);
  if (now - lastSched > sched_cycle)
    SchedLoop(now);
  if (pid_enabled) {
    autopid.run();
    output = relay;
  }
  digitalWrite(OUTPUT_PIN, output);
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
