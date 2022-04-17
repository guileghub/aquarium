#ifndef DS18B20TemperatureMeter_HH
#define DS18B20TemperatureMeter_HH
#include <vector>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Temperature.hh"
#include "Recorder.hh"
#include "log.hh"
#include "config.h"

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#if !defined(TEMP_HIST_LEN) || !defined(TEMP_HIST_PERIOD) || !defined(TEMPERATURE_ONE_WIRE_BUS_PIN)
#error defines missing
#endif

struct dallas_temp_Device {
  DeviceAddress dev_addr;
  String name;
  Recorder<time_t, Temperature> history;
  dallas_temp_Device();
};

struct DallasTempBus {
  OneWire oneWire;
  DallasTemperature dallas_temp;
  std::vector<dallas_temp_Device> devices;
  static String GetAddressToString(DeviceAddress deviceAddress);

  DallasTempBus();
};

void setup_temp_record();

void loop_temp_record();

DallasTempBus &GetTempBus();

#endif
