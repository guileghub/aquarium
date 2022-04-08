/*
includes needed to define function prototypes needs to be included here, in order to appear in the beginning of the arduino concatenated file.
otherwise the generated funtion declaration will use a include that is included after its definition.
*/
#include <WebSockets.h>
#define SCHED_NUM 3

// 1 week
#define TEMP_HIST_LEN (7*24*60/5)
// @5min
#define TEMP_HIST_PERIOD 5*60
#define TEMPERATURE_ONE_WIRE_BUS_PIN D2
#define TEMP_PID_CTRL_OUTPUT_PIN D5

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146

void setup() {
  setup_watchdog();
  setup_log();
  setup_wifi();
  setup_ota();
  setup_ntp();
  setup_temp_record();
  setup_schedule_power_ctrl();
  setup_temp_pid_ctrl();
  yield();
}

void loop() {
  loop_watchdog();
  loop_ota();
  loop_ntp();
  loop_temp_record();
  loop_schedule_power_ctrl();
  loop_pid_ctrl();
  loop_WEB();
}
