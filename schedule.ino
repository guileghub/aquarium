#include <vector>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include "uptime.h"

const int sched_cycle = 60;
time_t last_sched_status = 0;

const int temp_cycle = 1; // @1s
unsigned char *temps = nullptr;

void power_control(int port, bool val) {
  char port_code;
  switch (port) {
    case 1:
      port_code = '1';
      break;
    case 2:
      port_code = '2';
      break;
    case 3:
      port_code = '3';
      break;
    default:
      port_code = '0'; // all
  }
  String cmd = "C";
  cmd += port_code;
  cmd += val ? '1' : '0';
  String log = "Sending command: ";
  log += cmd;
  Log(log);
  Serial1.print(cmd);
}

void port2json(char *output, JsonArray vals) {
  for (int x = 0; x < SCHED_NUM; x++ ) {
    switch (output[x]) {
      case 0:
        vals[x] = false;
        continue;
      case 1:
        vals[x] = true;
        continue;
      default:
        vals[x] = nullptr;
        continue;
    }
  }
}

String ports_asjson() {
  DynamicJsonDocument json(1024);
  JsonArray cp = json.createNestedArray("CurrentPower");
  JsonArray fp = json.createNestedArray("SelectedPower");
  JsonArray sp = json.createNestedArray("ScheduledPower");
  port2json(current_output, cp);
  port2json(selected_output, fp);
  port2json(sched_output, sp);
  String pstatus;
  serializeJson(json, pstatus);
  return pstatus;
}
void broadcast_ports() {
  String pstatus(ports_asjson());
  broadcast_message(pstatus);
}

void setup_schedule_power_ctrl() {
  Serial1.begin(9600);
}

void broadcast_time(time_t t) {
  String ts(F("{\"time\":\""));
  ts += toISOString(t);
  uptime::calculateUptime();
  ts += "\",\"uptime\":{\"days\":";
  ts += uptime::getDays();
  ts += ",\"hours\":";
  ts += uptime::getHours();
  ts += ",\"minutes\":";
  ts += uptime::getMinutes();
  ts += ",\"seconds\":";
  ts += uptime::getSeconds();
  ts += "}}";
  broadcast_message(ts);
}

void loop_schedule_power_ctrl() {
  time_t t = now();
  if (t == last_sched_status)
    return;
  last_sched_status = t;
  broadcast_time(t);
  int h = hour(t);
  int m = minute(t);
  int s = second(t);
  int sched_new[SCHED_NUM];
  sched_output[0] = (h <= 9 || h >= 21); // 0-6 18-24
  sched_output[1] = (h > 9 && h < 21); // 6-18
  sched_output[2] = (h >= 22 || h <= 2); // 19-23
  for (unsigned x = 0; x < SCHED_NUM; x++) {
    if (selected_output[x] != 2)
      sched_new[x] = selected_output[x];
    else
      sched_new[x] = sched_output[x];
    if (sched_new[x] != current_output[x]) {
      current_output[x] = sched_new[x];
      power_control(x + 1, current_output[x]);
      break; // 1s between changed for each channel
    }
    if (m % 10 == x && s == x) {
      current_output[x] = sched_new[x];
      power_control(x + 1, current_output[x]);
      break; // 1s between changed for each channel
    }
  }
  if (t % 5)
    return;
  broadcast_ports();
#ifdef LOG
  String log = toISOString(t);
  log += " loop_schedule_power_ctrl: [";
  for (unsigned x = 0; x < SCHED_NUM; x++) {
    if (x) log += ',';
    log += static_cast<int>(current_output[x]);
  }
  log += ']';
  LOG(log);
#endif
}
