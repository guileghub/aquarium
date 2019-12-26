#include <FS.h>
#include <WebSocketsServer.h>
#include "HelperFunctions.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AutoPID.h>

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#define ONE_WIRE_BUS D4
#define OUTPUT_PIN D3
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

double temperature, setPoint;
bool relay;
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
  server.handleClient();
  webSocket.loop();
}
