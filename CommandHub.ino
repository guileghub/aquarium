#include <ArduinoJson.h>

void do_reboot();

void parse_message(uint8_t *payload, size_t length, std::function<void(String&)>reply) {
  String log;
  StaticJsonDocument<1024> json;
  DeserializationError error = deserializeJson(json, payload, length);
  if (error) {
    log = "deserializeJson() failed: ";
    log += error.f_str();
    Log(log);
    return;
  }
  JsonObject obj = json.as<JsonObject>();
  if (obj.isNull()) {
    Log("Error, JSON object expected.");
    return;
  }
  JsonVariant reboot = obj.getMember("reboot");
  if (reboot.is<bool>() && reboot.as<bool>())
    do_reboot();

  JsonVariant outputs = obj.getMember("SelectedPower");
  if (outputs.is<JsonArray>()) {
    for (int i = 0; i < SCHED_NUM; i++) {
      JsonVariant output = outputs.getElement(i);
      if (output.isNull()) {
        selected_output[i] = 2;
      } else {
        bool v = output.as<bool>();
        selected_output[i] = !!v;
      }
    }
    String res = ports_asjson();
    reply(res);
    return;
  }
  Log("unknown command");
}
