/* Go to http:// 192.168.4.1 in a web browser
   connected to this access point to see it.
*/

#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

// Below variables are used for Steinhart-Hart and associated temperature calcs
// Refer to // https://arduinodiy.wordpress.com/2015/11/10/measuring-temperature-with-ntc-the-steinhart-hart-formula/
#define            SERIESRESISTOR  150000    //  Reduced current and scale to 0 - 1 Volts
#define            NOMINAL_RESIST  10000     // @25C
#define            NOMINAL_TEMP    25        // See above
#define            BCOEFFICIENT    3950      // need to verify for Vishay 10K NTC
float              temperature   = 0.0;      //       ditto
int                temp_int      = 0;        //       ditto
// Below variables are general global variables
uint8_t            socketNumber;
char               AP_NameChar[6];            // AP_NameString.length() + 1];
const char         WiFiAPPSK[]   = "";        // "put your password here before compiling"
String             AP_NameString = "ESPAP";
String             temp_str;


ESP8266WebServer server(80);
WebSocketsServer webSocket(81);               // Create a Websocket server


void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

  switch (type) {
    case WStype_DISCONNECTED:
      // Reset the control for sending samples of ADC to idle to allow for web server to respond.
      Serial.printf("[%u] Disconnected!\n", num);
      yield();
      break;

    case WStype_CONNECTED: {                  // Braces required http://stackoverflow.com/questions/5685471/error-jump-to-case-label
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
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
  if (server.hasArg("download"))      return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html"))return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js"))  return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz"))  return "application/x-gzip";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
}


bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);

  if (path.endsWith("/"))
  {
    path += "counter.html";
  }

  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  Serial.println("PathFile: " + pathWithGz);

  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
  {
    if (SPIFFS.exists(pathWithGz)) path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  yield();
  return false;
}


void setupWiFi()
{
  WiFi.mode(WIFI_AP);
  // char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i = 0; i < AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);
  yield();
  WiFi.softAP(AP_NameChar, WiFiAPPSK);
}


void analogSample()
{

  temperature   = 0;
  temp_int      = round(temperature);
  temp_str      = String(temp_int);
}
