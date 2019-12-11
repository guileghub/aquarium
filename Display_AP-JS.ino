/* Create a WiFi access point and provide a web server on it so show temperature. 
   Originally published by markingle on http://www.esp8266.com/viewtopic.php?p=47535
   Refactored and enhanced for Hackster.io by: M. Ray Burnette 20160620
   Arduino 1.6.9 on Linux Mint 64-bit version 17.3 compiled: 20160706 by Ray Burnette
    Sketch uses 284,865 bytes (27%) of program storage space. Maximum is 1,044,464 bytes.
    Global variables use 38,116 bytes (46%) of dynamic memory, leaving 43,836 bytes for local variables. Maximum is 81,920 bytes.
*/

#include <FS.h>
#include <WebSocketsServer.h>
#include "HelperFunctions.h"
#include <OneWire.h>
#include <DallasTemperature.h>
// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#define ONE_WIRE_BUS D4 //Pin to which is attached a temperature sensor
#define ONE_WIRE_MAX_DEV 2 //The maximum number of devices
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
int numberOfDevices; //Number of temperature devices found
DeviceAddress devAddr[ONE_WIRE_MAX_DEV];  //An array device temperature sensors
float tempDev[ONE_WIRE_MAX_DEV]; //Saving the last measurement of temperature
float tempDevLast[ONE_WIRE_MAX_DEV]; //Previous temperature measurement
long lastTemp; //The last measurement
const int durationTemp = 5000; //The frequency of temperature measurement
//Convert device id to String
String GetAddressToString(DeviceAddress deviceAddress){
  String str = "";
  for (uint8_t i = 0; i < 8; i++){
    if( deviceAddress[i] < 16 ) str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
}
//Setting the temperature sensor
void SetupDS18B20(){
  DS18B20.begin();

  Serial.print("Parasite power is: "); 
  if( DS18B20.isParasitePowerMode() ){ 
    Serial.println("ON");
  }else{
    Serial.println("OFF");
  }
  
  numberOfDevices = DS18B20.getDeviceCount();
  Serial.print( "Device count: " );
  Serial.println( numberOfDevices );

  lastTemp = millis();
  DS18B20.requestTemperatures();

  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if( DS18B20.getAddress(devAddr[i], i) ){
      //devAddr[i] = tempDeviceAddress;
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: " + GetAddressToString(devAddr[i]));
      Serial.println();
    }else{
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }

    //Get resolution of DS18b20
    Serial.print("Resolution: ");
    Serial.print(DS18B20.getResolution( devAddr[i] ));
    Serial.println();

    //Read temperature from DS18b20
    float tempC = DS18B20.getTempC( devAddr[i] );
    Serial.print("Temp C: ");
    Serial.println(tempC);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  SPIFFS.begin();
  Serial.println(); Serial.print("Configuring access point...");
  setupWiFi();
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: "); Serial.println(myIP);

  server.on("/", HTTP_GET, []() {
    handleFileRead("/");
  });

  server.onNotFound([]() {                          // Handle when user requests a file that does not exist
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  webSocket.begin();                                // start webSocket server
  webSocket.onEvent(webSocketEvent);                // callback function

  server.begin();
  Serial.println("HTTP server started");

  SetupDS18B20();
  yield();
}

//Loop measuring the temperature
void TempLoop(long now){
  if( now - lastTemp > durationTemp ){ //Take a measurement at a fixed time (durationTemp = 5000ms, 5s)
    for(int i=0; i<numberOfDevices; i++){
      float tempC = DS18B20.getTempC( devAddr[i] ); //Measuring temperature in Celsius
      tempDev[i] = tempC; //Save the measured value to the array
      if(i==0){
        temp_str = String(tempC);        
        webSocket.sendTXT(socketNumber, "wpMeter,Arduino," + temp_str + ",1");
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
    static unsigned long l = 0;                     // only initialized once
    unsigned long t;                                // local var: type declaration at compile time
    
    t = millis();
  TempLoop( t );
//    if((t - l) > 5000) {                            // update temp every 5 seconds
//        analogSample(); yield();
//        webSocket.sendTXT(socketNumber, "wpMeter,Arduino," + temp_str + ",1");
//        l = t;                                      // typical runtime this IF{} == 300uS - 776uS measured
//        Serial.println(": " + temp_str + "C");
//        yield();
//    }

    server.handleClient();
    webSocket.loop();
}
