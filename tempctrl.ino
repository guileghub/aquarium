#include <OneWire.h>
#include <DallasTemperature.h>
#include <AutoPID.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>

/* Go to http:// 192.168.4.1 in a web browser connected to this access point to see it. */

uint8_t socketNumber;
char AP_NameChar[6];
const char WiFiAPPSK[] = "";
String AP_NameString = "ESPAP";

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);               // Create a Websocket server

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
      if (payload[0] == '#') {
        Serial.printf("[%u] get Text: %s\n", num, payload);
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
    path += "counter.html";
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

void setupWiFi() {
  WiFi.mode(WIFI_AP);
  // char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i = 0; i < AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);
  yield();
  WiFi.softAP(AP_NameChar, WiFiAPPSK);
}

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

double temperature = 0, setPoint = 0;
bool relay = false;
AutoPIDRelay autopid(&temperature, &setPoint, &relay, PWM_PERIOD, KP, KI, KD);

String GetAddressToString(DeviceAddress deviceAddress) {
  String str = "";
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16)
      str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
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

  // Loop through each device, print out address
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

    //Get resolution of DS18b20
    Serial.print("Resolution: ");
    Serial.print(DS18B20.getResolution(devAddr[i]));
    Serial.println();

    //Read temperature from DS18b20
    float tempC = DS18B20.getTempC(devAddr[i]);
    Serial.print("Temp C: ");
    Serial.println(tempC);
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

//Loop measuring the temperature
void TempLoop(long now) {
  if (now - lastTemp > cycle) {
    for (int i = 0; i < numberOfDevices; i++) {
      float tempC = DS18B20.getTempC(devAddr[i]); //Measuring temperature in Celsius
      tempDev[i] = tempC; //Save the measured value to the array
      if (i == 0) {
        String temp_str = String(tempC);
        webSocket.sendTXT(socketNumber,
                          "wpMeter,Arduino," + temp_str + ",1");
        Serial.print("Temp C: ");
        Serial.println(tempC);
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
  autopid.run();
  digitalWrite(OUTPUT_PIN, relay);
  //Serial.println("Saida: " + relay);
  server.handleClient();
  webSocket.loop();
}
