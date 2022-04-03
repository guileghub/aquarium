//#define TEMP_PID_CTRL
#define TEMP_RECORD
#define OTA_UPDATE
#define SCHED_POWER_CTRL
#define WEB
#define NTP

#ifdef SCHED_POWER_CTRL
#define SCHED_NUM 3
#endif

#ifdef TEMP_RECORD
// 1 week
#define TEMP_HIST_LEN (7*24*60/5)
// @5min
#define TEMP_HIST_PERIOD 5*60
#endif

#ifdef TEMP_RECORD
#define TEMPERATURE_ONE_WIRE_BUS_PIN D2
#endif

#ifdef TEMP_PID_CTRL
#define TEMP_PID_CTRL_OUTPUT_PIN D5
#endif

void setup() {
  ESP.wdtEnable(WDTO_8S);
  Serial.begin(115200);
  Serial1.begin(9600);
  delay(1000);
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
  setup_sched();
#endif

#ifdef TEMP_PID_CTRL
  setup_temp_pid_ctrl();
#endif
  yield();
}

void loop() {
  ESP.wdtFeed();

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
  SchedLoop(now);
#endif

#ifdef TEMP_PID_CTRL
  loop_pid_ctrl();
#endif

#ifdef WEB
  loop_WEB();
#endif
}
