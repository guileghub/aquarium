#define TEMP_RECORD
#ifdef TEMP_RECORD

TemperatureRecord::TemperatureRecord() :
		value(0xFF) {
}
TemperatureRecord::TemperatureRecord(float temp) {
	temp += 5;
	temp *= 4;
	if (temp >= 0 && temp < 255)
		value = static_cast<unsigned char>(temp);
	else
		value = 0xFF;
}
TemperatureRecord::operator float() const {
	if (value == 0xFF)
		return NAN;
	float res = value;
	res /= 4;
	res -= 5;
	return res;
}
TemperatureRecord::operator bool() const {
	return value != 0xFF;
}

void TemperatureRecorder::clear() {
	current = 0, lastRecordEpochTime = 0;
	hist.clear();
	Log("clear");
}
TemperatureRecorder::TemperatureRecorder(size_t time_interval, size_t capacity) :
		time_interval(time_interval), capacity(capacity) {
	clear();
}
void TemperatureRecorder::addRecord(TemperatureRecord const &t) {
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
void TemperatureRecorder::updateRecord(TemperatureRecord const &t) {
	if (hist.empty())
		addRecord(t);
	else
		hist[current - 1] = t;
}
void TemperatureRecorder::record(TemperatureRecord const &t,
		unsigned long epochTime) {
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
			addRecord (TemperatureRecord());lastRecordEpochTime = epochTime;
		}
	updateRecord(t);
	log = "hist.size()=";
	log += hist.size();
	log += " current=";
	log += current;
	Log(log);
}
std::vector<TemperatureRecord> TemperatureRecorder::query(unsigned long begin,
		unsigned long end) {
	std::vector < TemperatureRecord > result;
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

#endif
