const char* ssid = "Canopus";
const char* password = "B@r@lh@d@";
const char* hostname = "aquario";
IPAddress local_IP = INADDR_NONE; //(192, 168, 15, 254);
IPAddress gateway = INADDR_NONE; //(192, 168, 15, 1);
IPAddress subnet = INADDR_NONE; //(255, 0, 0, 0);
IPAddress primaryDNS = INADDR_NONE; //(192, 168, 15, 1);   //optional
IPAddress secondaryDNS = INADDR_NONE; //(8, 8, 4, 4); //optional

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
    delay(5000);
    ESP.restart();
  }
  Serial.print("Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}
