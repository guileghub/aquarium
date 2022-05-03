#include <ArduinoJson.h>
#include <limits>
#include "DS18B20TemperatureMeter.hh"
#include "iso_time.hh"

void do_reboot();

bool giveup_lowmem() {
  return false;//ESP.getMaxFreeBlockSize() < 4 * 1024;
}

void fill_temps(JsonDocument &response, time_type begin, time_type end) {
  JsonObject temps = response.createNestedObject("temperatures");
  size_t devs_num = GetTempBus().devices.size();
  JsonObject td[devs_num];
  for (int i = 0; i < devs_num; i++)
    td[i] = temps.createNestedObject(GetTempBus().devices[i].name);
  for (int i = 0; i < devs_num; i++) {
    std::vector<std::pair<time_type, Temperature>> tv = GetTempBus().devices[i].history.query(begin, end, giveup_lowmem);
#ifdef LOG
    String log;
    log += time_t_2_iso(begin);
    log += '|';
    log += time_t_2_iso(end);
    log += ' ';
    log += tv.size();
    LOG(log);
#endif
    for (auto t : tv) {
      String ts = time_t_2_iso(t.first);
      td[i][ts] = static_cast<float>(t.second);
#if 0
if (response.memoryUsage() + 64 > response.capacity() || ESP.getMaxFreeBlockSize() < 2 * 1024)
        return;
#endif
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
    String s = gt["Start"] | "";
    String e = gt["End"] | "";
    time_type st = iso_2_time_t(s.c_str());
    time_type et = iso_2_time_t(e.c_str());
    if (!et) et = std::numeric_limits<time_type>::max();
    String r = temperatureQuery(st, et);
    reply(r);
    return;
  }
  Log("unknown command");
}
