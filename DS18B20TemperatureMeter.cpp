#include "DS18B20TemperatureMeter.hh"
#include <TimeLib.h>
#include "iso_time.hh"

time_t last_temp = 0;

dallas_temp_Device::dallas_temp_Device() :
  history(TEMP_HIST_PERIOD, TEMP_HIST_LEN) {
}

String DallasTempBus::GetAddressToString(DeviceAddress deviceAddress) {
  String str = "";
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16)
      str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
}

DallasTempBus::DallasTempBus():
  oneWire(TEMPERATURE_ONE_WIRE_BUS_PIN),	dallas_temp(&oneWire) {
  dallas_temp.begin();
  dallas_temp.setWaitForConversion(false);
  dallas_temp.setResolution(10);
#ifdef LOG
  String log = "Parasite power is: ";
  if (dallas_temp.isParasitePowerMode()) {
    log += "ON";
  } else {
    log += "OFF";
  }
  LOG(log);
#endif
  dallas_temp.requestTemperatures();
  int numberOfDevices = dallas_temp.getDeviceCount();
#ifdef LOG
  log += "Device count: ";
  log += numberOfDevices;
  LOG(log);
#endif
  if (numberOfDevices > 0) {
    devices.clear();
    devices.resize(numberOfDevices);
  }

  for (int i = 0; i < numberOfDevices; i++) {
    // Search the wire for address
    if (dallas_temp.getAddress(devices[i].dev_addr, i)) {
      devices[i].name = GetAddressToString(devices[i].dev_addr);
#ifdef LOG
      log += "Found device ";
      log += i;
      log += "DEC";
      log += " with address: ";
      log += GetAddressToString(devices[i].dev_addr);
#endif
    } else {
      devices[i].name = "unknown";
#ifdef LOG
      log += "Found ghost device at ";
      log += i;
      log += "DEC";
      log += " but could not detect address. Check power and cabling";
#endif
    }
#ifdef LOG
    log += " Resolution: ";
    log += dallas_temp.getResolution(devices[i].dev_addr);
    LOG(log);
#endif
  }
}

#if 0
void fill_temps(JsonDocument &response, time_type begin, time_type end) {
  //week temp records
  JsonObject temps = response.createNestedObject("temperatures");
  size_t devs_num = devices.size();
  JsonArray td[devs_num];
  for (int i = 0; i < devs_num; i++)
    td[i] = temps.createNestedArray(devices[i].name);
  for (int i = 0; i < devs_num; i++) {
    std::vector<TemperatureRecord> tv = devices[i].history.query(begin, end);
    for (auto t : tv) {
      td[i].add(static_cast<float>(t));
      if (response.memoryUsage() + 64 > response.capacity())
        return;
    }
  }
}

void temperatureUpdate(time_type time_epoch) {
  if (!connected)
    return;
  DynamicJsonDocument response(4096);
  // fill_temps(response, time_epoch - 60, time_epoch);
  String message;
  serializeJson(response, message);
  webSocket.broadcastTXT(message);
}
#endif

void setup_temp_record() {

}

//Loop measuring the temperature
void loop_temp_record() {
  time_t time_epoch = now();
  if (time_epoch == last_temp)
    return;
  int numberOfDevices = GetTempBus().devices.size();
  for (int i = 0; i < numberOfDevices; i++) {
    float tempC = GetTempBus().dallas_temp.getTempC(GetTempBus().devices[i].dev_addr);
    Temperature t(tempC);
#ifdef LOG
    if (0 == second(time_epoch) % 5) {
      String log = time_t_2_iso(time_epoch);
      log += " Temperature[";
      log += GetTempBus().devices[i].name;
      log += "]=";
      log += tempC;
      log += "C";
      Log(log);
    }
#endif

    if (!second(time_epoch)) {
      GetTempBus().devices[i].history.record(t, time_epoch);
#ifdef LOG
      String log = time_t_2_iso(time_epoch);
      log += " GetTempBus().devices[";
      log += GetTempBus().devices[i].name;
      log += "].history records:";
      log += GetTempBus().devices[i].history.history.size();
      Log(log);
#endif
    }
  }
  GetTempBus().dallas_temp.requestTemperatures();
  //temperatureUpdate(time_epoch);
  last_temp = time_epoch;  //Remember the last time measurement
}

DallasTempBus &GetTempBus() {
  static DallasTempBus temp_bus;
  return temp_bus;
}
