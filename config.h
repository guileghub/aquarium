#ifndef CONFIG_H
#define CONFIG_H
#define SCHED_NUM 3

#define SSID "Canopus"
#define WLAN_PASSWD "B@r@lh@d@"
#ifdef ARDUINO_ARCH_ESP8266
#define HOSTNAME "aquarium_esp16"
#elif defined(ARDUINO_ARCH_ESP32)
#define HOSTNAME "aquarium_esp32"
#endif

#define NTP_SERVER "br.pool.ntp.org"

// 1 week
#define TEMP_HIST_LEN (7*24*60/5)
// @5min
#define TEMP_HIST_PERIOD 5*60
#ifdef ARDUINO_ARCH_ESP8266
#define TEMPERATURE_ONE_WIRE_BUS_PIN D2
//#define TEMP_PID_CTRL_OUTPUT_PIN
#elif defined(ARDUINO_ARCH_ESP32)
#define TEMPERATURE_ONE_WIRE_BUS_PIN 13
//#define TEMP_PID_CTRL_OUTPUT_PIN D5
#endif

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#endif
