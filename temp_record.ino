#ifdef TEMP_RECORD
#include <OneWire.h>
#include <DallasTemperature.h>
#include <vector>

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#define TEMPERATURE_ONE_WIRE_BUS_PIN D2
OneWire oneWire(TEMPERATURE_ONE_WIRE_BUS_PIN);
DallasTemperature DS18B20(&oneWire);

long last_temp; //The last measurement

struct TempMeasure {
  // 0 -> -5, 1->-4.75 2-> -4.5 254->58.5 255->invalid
  unsigned char value;
  TempMeasure(): value(0xFF) {
  }
  TempMeasure(float temp) {
    if (temp > -5 && temp <= 58.5) {
      temp += 5;
      temp *= 4;
      if (temp < 0) temp = 0;
      if (temp > 254) temp = 254;
      value = static_cast<unsigned char>(temp);
    } else value = 0xFF;
  }
  operator float() const {
    if (value == 0xFF)
      return NAN;
    float res = value;
    res /= 4;
    res -= 5;
    return res;
  }
  operator bool() const {
    return value != 0xFF;
  }
};
struct TempHistory {
  std::vector<TempMeasure> hist;
  size_t time_interval;
  size_t capacity;
  size_t current;
  unsigned long lastRecordEpochTime;
  void clear() {
    current = 0, lastRecordEpochTime = 0;
    hist.clear();
    Log("clear");
  }
  TempHistory(size_t time_interval, size_t capacity): time_interval(time_interval), capacity(capacity) {
    clear();
  }
  void addRecord(TempMeasure const& t) {
    if (hist.size() <= capacity) {
      hist.push_back(t);
      current = hist.size();
    } else {
      if (current >= capacity)
        current = 0;
      hist[current] = t;
      current++;
    }
  }
  void updateRecord(TempMeasure const& t) {
    if (hist.empty())
      addRecord(t);
    else
      hist[current - 1] = t;
  }
  void record(TempMeasure const& t, unsigned long epochTime) {
    String log;
    log += "epochTime: ";
    log += epochTime;
    log += " lastRecordEpochTime:";
    log += lastRecordEpochTime;
    if (epochTime < lastRecordEpochTime)
      clear();
    if (!lastRecordEpochTime)
      lastRecordEpochTime = epochTime;
    unsigned long gap = (epochTime - lastRecordEpochTime) / time_interval;
    log += " gap:";
    log += gap;
    Log(log);
    if (gap >= capacity) {
      clear();
      gap = 0;
    }
    if (gap) {
      while (gap--)
        addRecord(TempMeasure());
      lastRecordEpochTime = epochTime;
    }
    updateRecord(t);
    log = "hist.size()=";
    log += hist.size();
    log += " current=";
    log += current;
    Log(log);
  }
  std::vector<TempMeasure> query(unsigned long begin, unsigned long end) {
    std::vector<TempMeasure> result;
    size_t size = hist.size();
    long deltaBegin = (lastRecordEpochTime - begin) / time_interval;
    if (deltaBegin >= size)
      return result;
    long deltaEnd = (lastRecordEpochTime - end) / time_interval;
    if (deltaEnd >= size)
      return result;
    size_t i, e;
    if (deltaBegin < (size - current)) {
      i = size - current + deltaBegin;
    } else {
      i = deltaBegin + current - size;
    }
    if (deltaEnd < (size - current)) {
      e = size - current + deltaEnd;
    } else {
      e = deltaEnd + current - size;
    }
    for (; i != e; i++) {
      result.push_back(hist[i]);
      if (i >= size)
        i = 0;
    }
    return result;
  }
};

struct TempDevice {
  DeviceAddress dev_addr;
  String name;
  TempHistory history;
  TempDevice() : history(TEMP_HIST_PERIOD, TEMP_HIST_LEN) {
  }
};

std::vector<TempDevice> temp_devs;

void setup_temp_record() {
  SetupDS18B20();
  loop_temp_record();
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
String GetAddressToString(DeviceAddress deviceAddress) {
  String str = "";
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16)
      str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
}
#endif
void fill_temps(JsonDocument &response, unsigned long begin, unsigned long end) {
  //week temp records
  JsonObject temps = response.createNestedObject("temperatures");
  size_t devs_num = temp_devs.size();
  JsonArray td[devs_num];
  for (int i = 0; i < devs_num; i++)
    td[i] = temps.createNestedArray(temp_devs[i].name);
  for (int i = 0; i < devs_num; i++) {
    std::vector<TempMeasure> tv = temp_devs[i].history.query(begin, end);
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
  fill_temps(response, time_epoch - 60, time_epoch);
  String message;
  serializeJson(response, message);
  webSocket.sendTXT(web_sock_number, message);
}
//Loop measuring the temperature
void loop_temp_record() {
  int numberOfDevices = temp_devs.size();
  for (int i = 0; i < numberOfDevices; i++) {
    float tempC = DS18B20.getTempC(temp_devs[i].dev_addr); //Measuring temperature in Celsius
    TempMeasure t(tempC);
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
  temperatureUpdate(time_epoch);
  last_temp = time_epoch;  //Remember the last time measurement
}
#endif
