#include <vector>
#include <OneWire.h>
#include <DallasTemperature.h>
//typedef uint8_t DeviceAddress[8];
#include "Temperature.hh"
#include "Recorder.hh"

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#if !defined(TEMP_HIST_LEN) || !defined(TEMP_HIST_PERIOD) || !defined(TEMPERATURE_ONE_WIRE_BUS_PIN)
#error defines missing
#endif

struct dallas_temp_Device {
  DeviceAddress dev_addr;
  String name;
  Recorder<Temperature> history;
  dallas_temp_Device() :
    history(TEMP_HIST_PERIOD, TEMP_HIST_LEN) {
  }
};

struct dallas_temp_Bus {
  OneWire oneWire;
  DallasTemperature dallas_temp;
  std::vector<dallas_temp_Device> devices;
  static String GetAddressToString(DeviceAddress deviceAddress) {
    String str = "";
    for (uint8_t i = 0; i < 8; i++) {
      if (deviceAddress[i] < 16)
        str += String(0, HEX);
      str += String(deviceAddress[i], HEX);
    }
    return str;
  }

  dallas_temp_Bus():
    oneWire(TEMPERATURE_ONE_WIRE_BUS_PIN),	dallas_temp(&oneWire) {
    dallas_temp.begin();
    dallas_temp.setWaitForConversion(true);
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

    dallas_temp.requestTemperatures();

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
};

#if 0
void fill_temps(JsonDocument &response, unsigned long begin, unsigned long end) {
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

void temperatureUpdate(unsigned long time_epoch) {
  if (!connected)
    return;
  DynamicJsonDocument response(4096);
  // fill_temps(response, time_epoch - 60, time_epoch);
  String message;
  serializeJson(response, message);
  webSocket.broadcastTXT(message);
}
#endif

//dallas_temp_Bus temp_bus;

time_t last_temp = 0;

void setup_temp_record() {

}

//Loop measuring the temperature
void loop_temp_record() {
  unsigned long time_epoch = now();
  #if 0
  int numberOfDevices = temp_bus.devices.size();
  for (int i = 0; i < numberOfDevices; i++) {
    float tempC = temp_bus.dallas_temp.getTempC(temp_bus.devices[i].dev_addr);
    Temperature t(tempC);
#ifdef LOG
    String log;
    log += "Temperature[";
    log += temp_bus.devices[i].name;
    log += "]@(";
    log += time_epoch;
    log += ")=";
    log += tempC;
    log += "C";
    Log(log);
#endif
    temp_bus.devices[i].history.record(t, time_epoch);
  }
  temp_bus.dallas_temp.requestTemperatures();
  //temperatureUpdate(time_epoch);
  #endif
  last_temp = time_epoch;  //Remember the last time measurement
}
