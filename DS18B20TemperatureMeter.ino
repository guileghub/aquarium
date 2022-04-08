#ifdef RECORD_TEMPERATURE
#include <vector>
#include <OneWire.h>
#include <DallasTemperature.h>
//typedef uint8_t DeviceAddress[8];
#include "Temperature.hh"
#include "Recorder.hh"

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146

struct DS18B20_Device {
  DeviceAddress dev_addr;
  String name;
  Recorder<Temperature> history;
  DS18B20_Device() :
    history(TEMP_HIST_PERIOD, TEMP_HIST_LEN) {
  }
};

struct DS18B20_Bus {
  OneWire oneWire;
  DallasTemperature DS18B20;
  std::vector<DS18B20_Device> devices;
  static String GetAddressToString(DeviceAddress deviceAddress) {
    String str = "";
    for (uint8_t i = 0; i < 8; i++) {
      if (deviceAddress[i] < 16)
        str += String(0, HEX);
      str += String(deviceAddress[i], HEX);
    }
    return str;
  }

  DS18B20_Bus():
    oneWire(TEMPERATURE_ONE_WIRE_BUS_PIN),	DS18B20(&oneWire) {
    DS18B20.begin();
    DS18B20.setWaitForConversion(true);
    DS18B20.setResolution(10);
#ifdef LOG
    String log = "Parasite power is: ";
    if (DS18B20.isParasitePowerMode()) {
      log += "ON";
    } else {
      log += "OFF";
    }
    LOG(log);
#endif
    int numberOfDevices = DS18B20.getDeviceCount();
#ifdef LOG
    log += "Device count: ";
    log += numberOfDevices;
    LOG(log);
#endif
    if (numberOfDevices > 0) {
      devices.clear();
      devices.resize(numberOfDevices);
    }

    DS18B20.requestTemperatures();

    for (int i = 0; i < numberOfDevices; i++) {
      // Search the wire for address
      if (DS18B20.getAddress(devices[i].dev_addr, i)) {
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
      log += DS18B20.getResolution(devices[i].dev_addr);
      LOG(log);
#endif
    }
  }
};

float TemperatureCelsius() {
  return DS18B20.getTempC(dev_addr);
}

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

DS18B20_Bus temp_bus;

//Loop measuring the temperature
void loop_temp_record() {
  unsigned long time_epoch = now();
  int numberOfDevices = temp_bus.devices.size();
  for (int i = 0; i < numberOfDevices; i++) {
    float tempC = DS18B20.getTempC(temp_bus.devices[i].dev_addr); //Measuring temperature in Celsius
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
  DS18B20.requestTemperatures();
  //temperatureUpdate(time_epoch);
  last_temp = time_epoch;  //Remember the last time measurement
}
#endif
