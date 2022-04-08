#define WEB
#define NTP
#define OTA_UPDATE
#define RECORD_TEMPERATURE
#define SCHED_POWER_CTRL
//#define TEMP_PID_CTRL

#ifdef SCHED_POWER_CTRL
#define SCHED_NUM 3
#endif

#ifdef TEMP_RECORD
#ifndef NTP
#error TEMP_RECORD requires NTP service
#endif
// 1 week
#define TEMP_HIST_LEN (7*24*60/5)
// @5min
#define TEMP_HIST_PERIOD 5*60
#define TEMPERATURE_ONE_WIRE_BUS_PIN D2
#endif

#ifdef SCHED_POWER_CTRL
#ifndef NTP
#error SCHED_POWER_CTRL requires NTP service
#endif
#endif

#ifdef TEMP_PID_CTRL
#ifndef TEMP_RECORD
#error TEMP_PID_CTRL requires TEMP_RECORD service
#endif
#define TEMP_PID_CTRL_OUTPUT_PIN D5
#endif

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#define TEMPERATURE_ONE_WIRE_BUS_PIN D2

void setup() {
#ifdef WATCHDOG_ENABLED
  setup_watchdog()
#endif
  setup_log();
  setup_wifi();
#ifdef OTA_UPDATE
  setup_ota();
#endif
#ifdef NTP
  setup_ntp();
#endif
#ifdef TEMP_RECORD
  setup_temp_record();
#endif
#ifdef SCHED_POWER_CTRL
  setup_schedule_power_ctrl();
#endif
#ifdef TEMP_PID_CTRL
  setup_temp_pid_ctrl();
#endif
  yield();
}

void loop() {
#ifdef WATCHDOG_ENABLED
  loop_watchdog();
#endif
#ifdef OTA_UPDATE
  loop_ota();
#endif
#ifdef NTP
  loop_ntp();
#endif
#ifdef TEMP_RECORD
  loop_temp_record();
#endif
#ifdef SCHED_POWER_CTRL
  loop_schedule_power_ctrl();
#endif
#ifdef TEMP_PID_CTRL
  loop_pid_ctrl();
#endif
#ifdef WEB
  loop_WEB();
#endif
}
