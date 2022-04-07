#ifdef TEMP_RECORD
#include <OneWire.h>
#include <DallasTemperature.h>
//typedef uint8_t DeviceAddress[8];
#include "TemperatureRecorder.hh"

// Interno: 2840e363121901e2
// Externo: 289a9a7512190146
#define TEMPERATURE_ONE_WIRE_BUS_PIN D2
OneWire oneWire(TEMPERATURE_ONE_WIRE_BUS_PIN);
DallasTemperature DS18B20(&oneWire);

struct DS18B20Device : public TemperatureMeter {
	DeviceAddress dev_addr;
	String name;
	TemperatureRecorder history;
	TempDevice() :
			history(TEMP_HIST_PERIOD, TEMP_HIST_LEN) {
	}
	float TemperatureCelsius(){
		return DS18B20.getTempC(dev_addr);
	}
};

std::vector<TempDevice> DS18B20_devices;

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
	DS18B20.setResolution(10);
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
		DS18B20_devices.clear();
		DS18B20_devices.resize(numberOfDevices);
	}

	DS18B20.requestTemperatures();

	for (int i = 0; i < numberOfDevices; i++) {
		// Search the wire for address
		if (DS18B20.getAddress(DS18B20_devices[i].dev_addr, i)) {
			DS18B20_devices[i].name = GetAddressToString(DS18B20_devices[i].dev_addr);
			log += "Found device ";
			log += i;
			log += "DEC";
			log += " with address: ";
			log += GetAddressToString(DS18B20_devices[i].dev_addr);
		} else {
			DS18B20_devices[i].name = "unknown";
			log += "Found ghost device at ";
			log += i;
			log += "DEC";
			log += " but could not detect address. Check power and cabling";
		}
		log += " Resolution: ";
		log += DS18B20.getResolution(DS18B20_devices[i].dev_addr);
		Log(log);
	}
}

#if 0
void fill_temps(JsonDocument &response, unsigned long begin, unsigned long end) {
  //week temp records
  JsonObject temps = response.createNestedObject("temperatures");
  size_t devs_num = DS18B20_devices.size();
  JsonArray td[devs_num];
  for (int i = 0; i < devs_num; i++)
    td[i] = temps.createNestedArray(DS18B20_devices[i].name);
  for (int i = 0; i < devs_num; i++) {
    std::vector<TemperatureRecord> tv = DS18B20_devices[i].history.query(begin, end);
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
//Loop measuring the temperature
void loop_temp_record() {
	unsigned long time_epoch = now();
	int numberOfDevices = DS18B20_devices.size();
	for (int i = 0; i < numberOfDevices; i++) {
		float tempC = DS18B20.getTempC(DS18B20_devices[i].dev_addr); //Measuring temperature in Celsius
		TemperatureRecord t(tempC);
		String log;
		log += "Temperature[";
		log += DS18B20_devices[i].name;
		log += "]@(";
		log += time_epoch;
		log += ")=";
		log += tempC;
		log += "C";
		Log(log);
		DS18B20_devices[i].history.record(t, time_epoch);
	}
	DS18B20.setWaitForConversion(true); //No waiting for measurement
	DS18B20.requestTemperatures(); //Initiate the temperature measurement
	//temperatureUpdate(time_epoch);
	last_temp = time_epoch;  //Remember the last time measurement
}
#endif
