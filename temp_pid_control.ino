#include <AutoPID.h>
#define TEMP_PID_CTRL_OUTPUT_PIN D5
double target_temperature = 0;
double current_temperature = 0;
#define PWM_PERIOD 1000
#define KP .12
#define KI .0003
#define KD 0

bool pid_enabled = false;
bool relay = false;
bool output = false;

AutoPIDRelay autopid(&current_temperature, &target_temperature, &relay, PWM_PERIOD, KP, KI, KD);

void setup_temp_pid_ctrl() {
  autopid.setBangBang(4);
  autopid.setTimeStep(temp_cycle);
  pinMode(TEMP_PID_CTRL_OUTPUT_PIN, OUTPUT);
}

#ifdef TEMP_PID_CTRL
if (pid_enabled) {
  status["targetTemperature"] = target_temperature;
}
status["power"] = !!output;
#endif
#ifdef TEMP_PID_CTRL
JsonVariant targetTemperature = obj.getMember("targetTemperature");
if (targetTemperature.is<double>()) {
  target_temperature = targetTemperature.as<double>();
  pid_enabled = true;
  log = String("targetTemperature=> pid_enabled=") + pid_enabled + ", output=" + output;
  Log(log);
  return;
}
JsonVariant power = obj.getMember("power");
if (power.is<bool>()) {
  pid_enabled = false;
  output = (bool) power;
  log = String("POWER=> pid_enabled=") + pid_enabled + ", output=" + output;
  Log(log);
  return;
}
#endif

void loop_pid_ctrl() {
  if (pid_enabled) {
    autopid.run();
    output = relay;
  }
  digitalWrite(TEMP_PID_CTRL_OUTPUT_PIN, output);
}
