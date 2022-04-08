#define TEMP_RECORD
#ifdef TEMP_RECORD
#include "Temperature.hh"
#include <cmath>

Temperature::Temperature() :
		value(0xFF) {
}
Temperature::Temperature(float temp) {
	temp += 5;
	temp *= 4;
	if (temp >= 0 && temp < 255)
		value = static_cast<unsigned char>(temp);
	else
		value = 0xFF;
}
Temperature::operator float() const {
	if (value == 0xFF)
		return NAN;
	float res = value;
	res /= 4;
	res -= 5;
	return res;
}
Temperature::operator bool() const {
	return value != 0xFF;
}

#endif
