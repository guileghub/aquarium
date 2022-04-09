#include <vector>
#include <TimeLib.h>

const int sched_cycle = 60;
time_t last_sched_status = 0;
int sched_output[SCHED_NUM] = { -1, -1, -1};

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

void setup_schedule_power_ctrl() {
  Serial1.begin(9600);
  loop_schedule_power_ctrl();
}
void loop_schedule_power_ctrl() {
  time_t t = now();
  if (t == last_sched_status)
    return;
  int h = hour(t);
  int m = minute(t);
  int s = second(t);
  int sched_new[SCHED_NUM];
  if (h < 7 || h > 23)
    sched_new[0] = true;
  else
    sched_new[0] = false;
  if (h > 7 || h < 23 )
    sched_new[1] = true;
  else
    sched_new[1] = false;
  if (h > 19 || h < 22)
    sched_new[2] = true;
  else
    sched_new[2] = false;

  for (unsigned x = 0; x < SCHED_NUM; x++) {
    if (sched_new[x] != sched_output[x]) {
      sched_output[x] = sched_new[x];
      power_control(x + 1, sched_output[x]);
    }
  }
  if (t % 5)
    return;
#ifdef LOG
  String log;
  int ye = year(t);
  int mo = month(t);
  int da = day(t);
  log += ye;
  log += '-';
  log += mo;
  log += '-';
  log += da;
  log += ' ';
  log += h;
  log += ':';
  log += m;
  log += ':';
  log += s;
  log += " loop_schedule_power_ctrl : [";
  for (unsigned x = 0; x < SCHED_NUM; x++) {
    if (x) log += ',';
    log += sched_output[x];
  }
  log += ']';
  LOG(log);
#endif
  last_sched_status = t;
}
