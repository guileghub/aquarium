#include "config.h"

void setup_wifi() {
  Serial.println(F("\nConfiguring access point..."));
  IPAddress local_IP;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress primaryDNS;
  IPAddress secondaryDNS;
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println(F("STA Failed to configure"));
  }
  WiFi.setHostname(HOSTNAME);
  Serial.println(F("WiFi connecting."));
  WiFi.begin(SSID, WLAN_PASSWD);
  /*
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    } */
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(F("Connection Failed! Rebooting..."));
    ESP.restart();
  }
  Serial.print(F("Ready! Use 'http://"));
  Serial.print(WiFi.localIP());
  Serial.println(F("' to connect"));
}
