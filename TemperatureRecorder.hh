#define TEMP_RECORD
#ifdef TEMP_RECORD
#include <OneWire.h>
#include <DallasTemperature.h>
//typedef uint8_t DeviceAddress[8];
#include <vector>

OneWire oneWire(TEMPERATURE_ONE_WIRE_BUS_PIN);
DallasTemperature DS18B20(&oneWire);

class TemperatureRecord {
public:
  unsigned char value;
  TemperatureRecord();
  TemperatureRecord(float temp);
  operator float() const;
  operator bool() const;
};

class TemperatureMeter {
protected:
	virtual ~TemperatureMeter();
public:
	virtual float TemperatureCelsius()=0;
};

class TemperatureRecorder {
  std::vector<TemperatureRecord> history;
  size_t time_interval;
  size_t capacity;
  size_t current;
  time_t lastRecordEpochTime;
  TemperatureMeter&meter;
  void clear();
public:
  TemperatureRecorder(size_t time_interval, size_t capacity, TemperatureMeter&meter);
  void addRecord(TemperatureRecord const& t);
  void updateRecord(TemperatureRecord const& t);
  void record(TemperatureRecord const& t, time_t epochTime);
  std::vector<TemperatureRecord> query(time_t begin, time_t end);
};

struct TempDevice {
  DeviceAddress dev_addr;
  String name;
  TemperatureRecorder history;
  TempDevice(uint8_t pin, unsigned temp_period, unsigned temp_history_size) : history(temp_period, temp_history_size) {
  }
};

std::vector<TempDevice> temp_devs;

void setup_temp_record() {
  SetupDS18B20();
  loop_temp_record();
}


String GetAddressToString(DeviceAddress deviceAddress) {
  String str = "";
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16)
      str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
}
//Setting the temperature sensor
void SetupDS18B20() {
  DS18B20.begin();

  String log = "Parasite power is: ";
  if (DS18B20.isParasitePowerMode()) {
    log += "ON";
  } else {
    log += "OFF";
  }
  Log(log);

  int numberOfDevices = DS18B20.getDeviceCount();
  log += "Device count: ";
  log += numberOfDevices;
  Log(log);
  if (numberOfDevices > 0) {
    temp_devs.clear();
    temp_devs.resize(numberOfDevices);
  }

  DS18B20.requestTemperatures();

  for (int i = 0; i < numberOfDevices; i++) {
    // Search the wire for address
    if (DS18B20.getAddress(temp_devs[i].dev_addr, i)) {
      temp_devs[i].name = GetAddressToString(temp_devs[i].dev_addr);
      log += "Found device ";
      log += i;
      log += "DEC";
      log += " with address: ";
      log += GetAddressToString(temp_devs[i].dev_addr);
    } else {
      temp_devs[i].name = "unknown";
      log += "Found ghost device at ";
      log += i;
      log += "DEC";
      log +=
        " but could not detect address. Check power and cabling";
    }
    log += " Resolution: ";
    log += DS18B20.getResolution(temp_devs[i].dev_addr);
    Log(log);
  }
}

#if 0
void fill_temps(JsonDocument &response, time_t begin, time_t end) {
  //week temp records
  JsonObject temps = response.createNestedObject("temperatures");
  size_t devs_num = temp_devs.size();
  JsonArray td[devs_num];
  for (int i = 0; i < devs_num; i++)
    td[i] = temps.createNestedArray(temp_devs[i].name);
  for (int i = 0; i < devs_num; i++) {
    std::vector<TemperatureRecord> tv = temp_devs[i].history.query(begin, end);
    for (auto t : tv) {
      td[i].add(static_cast<float>(t));
      if (response.memoryUsage() + 64 > response.capacity())
        return;
    }
  }
}

void temperatureUpdate(time_t time_epoch) {
  if (!connected)
    return;
  DynamicJsonDocument response(4096);
  // fill_temps(response, time_epoch - 60, time_epoch);
  String message;
  serializeJson(response, message);
  webSocket.broadcastTXT(message);
}
#endif
//Loop measuring the temperature
void loop_temp_record() {
	time_t time_epoch = now();
  int numberOfDevices = temp_devs.size();
  for (int i = 0; i < numberOfDevices; i++) {
    float tempC = DS18B20.getTempC(temp_devs[i].dev_addr); //Measuring temperature in Celsius
    TemperatureRecord t(tempC);
    String log;
    log += "Temperature[";
    log += temp_devs[i].name;
    log += "]@(";
    log += time_epoch;
    log += ")=";
    log += tempC;
    log += "C";
    Log(log);
    temp_devs[i].history.record(t, time_epoch);
  }
  DS18B20.setWaitForConversion(false); //No waiting for measurement
  DS18B20.requestTemperatures(); //Initiate the temperature measurement
  //temperatureUpdate(time_epoch);
  last_temp = time_epoch;  //Remember the last time measurement
}
#endif
