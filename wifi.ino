IPAddress local_IP;
IPAddress gateway;
IPAddress subnet;
IPAddress primaryDNS;
IPAddress secondaryDNS;

void setup_wifi() {
  Serial.println("\nConfiguring access point...");
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.setHostname(hostname);
  Serial.println("WiFi connecting.");
  WiFi.begin(ssid, password);
  /*
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    } */
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    ESP.restart();
  }
  Serial.print("Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}
