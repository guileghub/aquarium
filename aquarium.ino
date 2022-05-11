/*
  includes needed to define function prototypes must be included here, in order to appear in the beginning of the arduino concatenated file.
  otherwise the generated funtion declaration will use a include that is included after its definition.
*/
// ARDUINO_ESP32_DEV -DARDUINO_ARCH_ESP32 "-DARDUINO_BOARD=\"ESP32_DEV\"" "-DARDUINO_VARIANT=\"doitESP32devkitV1\"" -DESP32
// -DARDUINO=10819 -DARDUINO_ESP8266_NODEMCU_ESP12E -DARDUINO_ARCH_ESP8266 "-DARDUINO_BOARD=\"ESP8266_NODEMCU_ESP12E\"" -DLED_BUILTIN=2 -DFLASHMODE_DIO -DESP8266 
#ifdef ARDUINO_ARCH_ESP8266
#else defined(DARDUINO_ARCH_ESP32)
#else Error board not supported
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "log.hh"
#include "DS18B20TemperatureMeter.hh"
#include <esp_system.h>
#include <rom/rtc.h>
#include "config.h"

char selected_output[SCHED_NUM] = { 2, 2, 2};
char sched_output[SCHED_NUM] = { 2, 2, 2};
char current_output[SCHED_NUM] = { 2, 2, 2};
typedef time_t time_type;

void do_reboot() {
  ESP.restart();
}

char *const reset_reason(RESET_REASON reason)
{
  switch ( reason)
  {
    case 1  : return "Vbat power on reset";
    case 3  : return "Software reset digital core";
    case 4  : return "Legacy watch dog reset digital core";
    case 5  : return "Deep Sleep reset digital core";
    case 6  : return "Reset by SLC module, reset digital core";
    case 7  : return "Timer Group0 Watch dog reset digital core";
    case 8  : return "Timer Group1 Watch dog reset digital core";
    case 9  : return "RTC Watch dog Reset digital core";
    case 10 : return "Instrusion tested to reset CPU";
    case 11 : return "Time Group reset CPU";
    case 12 : return "Software reset CPU";
    case 13 : return "RTC Watch dog Reset CPU";
    case 14 : return "for APP CPU, reseted by PRO CPU";
    case 15 : return "Reset when the vdd voltage is not stable";
    case 16 : return "RTC Watch dog reset digital core and rtc module";
    default : return "Unknown";
  }
}

void broadcast_boardinfo() {
  DynamicJsonDocument bi(1024);
  //bi["chipId"] = ESP.getChipId();
  bi["cpuFreqMHz"] = ESP.getCpuFreqMHz();
  bi["resetReason"] = reset_reason(rtc_get_reset_reason(0));
  bi["resetReason1"] = reset_reason(rtc_get_reset_reason(1));
  bi["freeHeap"] = ESP.getFreeHeap();
//  bi["heapFragmentation"] = ESP.getHeapFragmentation();
//  bi["maxFreeBlockSize"] = ESP.getMaxFreeBlockSize();
  String m;
  serializeJson(bi, m);
  broadcast_message(m);
}

time_t l;
void setup() {
  setup_watchdog();
  setup_log();
  setup_wifi();
  setup_ota();
  setup_ntp();
  setup_temp_record();
  setup_schedule_power_ctrl();
  //setup_temp_pid_ctrl();
  setup_WEB();
  yield();
  l = now();
}

void loop() {
  loop_watchdog();
  loop_ota();
  if (!loop_ntp()) {
    Log("Time not sync yet, waiting.");
    return;
  }
  loop_temp_record();
  loop_schedule_power_ctrl();
  //loop_pid_ctrl();
  loop_WEB();
  time_t t = now();
  if (t == l)
    return;
  l = t;
}
