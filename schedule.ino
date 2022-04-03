#ifdef TEMP_RECORD
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AutoPID.h>
#include <vector>

const int sched_cycle = 60;
int sched_output[SCHED_NUM] = { -1, -1, -1};

long lastTemp, lastSched; //The last measurement
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

#error  SchedLoop(now);
void SchedLoop(unsigned long now) {
  String log = "SchedLoop ";
  log += timeClient.getFormattedTime();
  Log(log);
  // if(timeClient.isTimeSet()){
  int hour = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int sched_new[SCHED_NUM];
  if (hour > 17 || hour < 5)
    sched_new[0] = true;
  else
    sched_new[0] = false;
  if (hour > 6 || hour < 18)
    sched_new[1] = false;
  else
    sched_new[1] = true;
  sched_new[2] = sched_output[2];

  for (unsigned x = 0; x < SCHED_NUM; x++) {
    if (sched_new[x] != sched_output[x]) {
      sched_output[x] = sched_new[x];
      power_control(x + 1, sched_output[x]);
    }
  }
  lastSched = now;
  schedUpdate(now);
}
#endif
