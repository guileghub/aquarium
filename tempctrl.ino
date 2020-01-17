#include <OneWire.h>
#include <DallasTemperature.h>
#include <AutoPID.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <Arduino_JSON.h>
// https://github.com/arduino-libraries/Arduino_JSON/blob/master/examples/JSONObject/JSONObject.ino

/* Go to http:// 192.168.4.1 in a web browser connected to this access point to see it. */

uint8_t socketNumber;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#define ONE_WIRE_BUS D4
#define OUTPUT_PIN D5
#define ONE_WIRE_MAX_DEV 2 //The maximum number of devices
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
int numberOfDevices;
DeviceAddress devAddr[ONE_WIRE_MAX_DEV];
float tempDev[ONE_WIRE_MAX_DEV]; //Saving the last measurement of temperature
float tempDevLast[ONE_WIRE_MAX_DEV]; //Previous temperature measurement
long lastTemp; //The last measurement
const int cycle = 1000;

#define PWM_PERIOD 1000
#define KP .12
#define KI .0003
#define KD 0

double current_temperature = 0, target_temperature = 0;
bool pid_enabled = false;
bool relay = false;
bool output = false;

AutoPIDRelay autopid(&current_temperature, &target_temperature, &relay, PWM_PERIOD, KP, KI, KD);

void setupWiFi() {
  WiFi.mode(WIFI_AP);
  yield();
  WiFi.softAP("TC", "B@r@lh@d@", 1, true);
}

//Setting the temperature sensor
void SetupDS18B20() {
  DS18B20.begin();

  Serial.print("Parasite power is: ");
  if (DS18B20.isParasitePowerMode()) {
    Serial.println("ON");
  } else {
    Serial.println("OFF");
  }

  numberOfDevices = DS18B20.getDeviceCount();
  Serial.print("Device count: ");
  Serial.println(numberOfDevices);
  if (numberOfDevices > ONE_WIRE_MAX_DEV)
    numberOfDevices = ONE_WIRE_MAX_DEV;

  lastTemp = millis();
  DS18B20.requestTemperatures();

  for (int i = 0; i < numberOfDevices; i++) {
    // Search the wire for address
    if (DS18B20.getAddress(devAddr[i], i)) {
      //devAddr[i] = tempDeviceAddress;
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: " + GetAddressToString(devAddr[i]));
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(
        " but could not detect address. Check power and cabling");
    }

    Serial.print("Resolution: ");
    Serial.print(DS18B20.getResolution(devAddr[i]));
    Serial.println();
  }
}

void SetupPID() {
  autopid.setBangBang(4);
  autopid.setTimeStep(cycle);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(OUTPUT_PIN, OUTPUT);
  SPIFFS.begin();
  Serial.println();
  Serial.print("Configuring access point...");
  setupWiFi();
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

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

  SetupDS18B20();
  SetupPID();
  yield();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      yield();
      break;

    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
                      ip[1], ip[2], ip[3], payload);
        yield();
        socketNumber = num;
        break;
      }

    case WStype_TEXT:
      {
        Serial.printf("[%u] get Text: %s\n", num, payload);
        String message((char*)payload);
        parse_message(message);
        yield();
      }
      break;

    case WStype_ERROR:
      Serial.printf("Error [%u] , %s\n", num, payload);
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
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
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
  String message = "{ \"currentTemperature\" :";
  message += current_temperature;
  if (pid_enabled) {
    message += ", \"targetTemperature\" :";
    message += target_temperature;
  }
  message += ", \"power\" :";
  message += output?"true":"false";
  message += "}";
  webSocket.sendTXT(socketNumber, message);
}

void parse_message(String m) {
  Serial.println(String("parse_message") + m);
  JSONVar message = JSON.parse(m);
  if (JSON.typeof(message) != "object") {
    Serial.println("Erro: " + m);
    return;
  }
  if (message.hasOwnProperty("targetTemperature")) {
    JSONVar targetTemperature = message["targetTemperature"];
    if (!(JSON.typeof(targetTemperature) == "number")) {
      Serial.println("Error, targetTemperature is not an number");
      return;
    }
    target_temperature = (double)targetTemperature;
    pid_enabled = true;
    Serial.println(String("targetTemperature=> pid_enabled=") + pid_enabled + ", output=" + output);
    return;
  }
  if (message.hasOwnProperty("power")) {
    JSONVar power = message["power"];
    if (!(JSON.typeof(power) == "boolean")) {
      Serial.println("Error, power is not an boolean");
      return;
    }
    pid_enabled = false;
    output = (bool) power;
    Serial.println(String("POWER=> pid_enabled=") + pid_enabled + ", output=" + output);
    return;
  }
}
//Loop measuring the temperature
void TempLoop(long now) {
  if (now - lastTemp > cycle) {
    for (int i = 0; i < numberOfDevices; i++) {
      float tempC = DS18B20.getTempC(devAddr[i]); //Measuring temperature in Celsius
      tempDev[i] = tempC; //Save the measured value to the array
      if (i == 0) {
        current_temperature = tempC;
        send_update();
      }
    }
    DS18B20.setWaitForConversion(false); //No waiting for measurement
    DS18B20.requestTemperatures(); //Initiate the temperature measurement
    lastTemp = millis();  //Remember the last time measurement
  }
}

void loop() {
  unsigned long t = millis();
  TempLoop(t);
  if (pid_enabled) {
    autopid.run();
    output = relay;
  }
  digitalWrite(OUTPUT_PIN, output);
  server.handleClient();
  webSocket.loop();
}
