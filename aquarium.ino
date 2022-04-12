/*
  includes needed to define function prototypes needs to be included here, in order to appear in the beginning of the arduino concatenated file.
  otherwise the generated funtion declaration will use a include that is included after its definition.
*/
#include <WebSockets.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "log.hh"
#define SCHED_NUM 3
char selected_output[SCHED_NUM] = { 2, 2, 2};
char sched_output[SCHED_NUM] = { 2, 2, 2};
char current_output[SCHED_NUM] = { 2, 2, 2};
typedef time_t time_type;

const char* ssid = "Canopus";
const char* password = "B@r@lh@d@";
const char* hostname = "aquarium";

// 1 week
#define TEMP_HIST_LEN (7*24*60/5)
// @5min
#define TEMP_HIST_PERIOD 5*60
#define TEMPERATURE_ONE_WIRE_BUS_PIN D2
#define TEMP_PID_CTRL_OUTPUT_PIN D5

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146

void do_reboot() {
  ESP.restart();
}

void broadcast_boardinfo() {
  DynamicJsonDocument bi(1024);
  bi["chipId"] = ESP.getChipId();
  bi["cpuFreqMHz"] = ESP.getCpuFreqMHz();
  bi["resetReason"] = ESP.getResetReason();
  bi["freeHeap"] = ESP.getFreeHeap();
  bi["heapFragmentation"] = ESP.getHeapFragmentation();
  bi["maxFreeBlockSize"] = ESP.getMaxFreeBlockSize();
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
