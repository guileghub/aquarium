#include <ArduinoJson.h>
#include <limits>
#include "DS18B20TemperatureMeter.hh"
#include "iso_string.hh"

void do_reboot();

bool giveup_lowmem() {
  return ESP.getMaxFreeBlockSize() < 16 * 1024;
}

void fill_temps(JsonDocument &response, time_type begin, time_type end) {
  JsonObject temps = response.createNestedObject("temperatures");
  size_t devs_num = GetTempBus().devices.size();
  JsonObject td[devs_num];
  for (int i = 0; i < devs_num; i++)
    td[i] = temps.createNestedObject(GetTempBus().devices[i].name);
  for (int i = 0; i < devs_num; i++) {
    std::vector<std::pair<time_type, Temperature>> tv = GetTempBus().devices[i].history.query(begin, end, giveup_lowmem);
    for (auto t : tv) {
      String ts = toISOString(t.first);
      td[i][ts] = static_cast<float>(t.second);
      if (response.memoryUsage() + 64 > response.capacity() || ESP.getMaxFreeBlockSize() < 8 * 1024)
        return;
    }
  }
}

String temperatureQuery(time_type begin, time_type end) {
  DynamicJsonDocument response(4096);
  fill_temps(response, begin, end);
  String message;
  serializeJson(response, message);
  return message;
}

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
  JsonVariant reboot = obj.getMember("Reboot");
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
  JsonVariant pps = obj.getMember("PortPowerStatus");
  if (!pps.isNull()) {
    broadcast_ports();
    return;
  }
  JsonVariant bi = obj.getMember("BoardInfo");
  if (!bi.isNull()) {
    broadcast_boardinfo();
    return;
  }
  JsonVariant gt = obj.getMember("GetTemperatures").as<JsonObject>();
  if (!gt.isNull()) {
    time_type st = 0, et = std::numeric_limits<time_type>::max();
    String s = gt["Start"] | "";
    String e = gt["End"] | "";
    String r = temperatureQuery(st, et);
    reply(r);
    return;
  }
  Log("unknown command");
}
