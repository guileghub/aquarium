#ifndef RECORDER_HH
#define RECORDER_HH
#include <vector>
#include <TimeLib.h>
#include "log.hh"

template<class Time, class Record> struct Recorder {
    std::vector<Record> history;
    size_t time_interval;
    size_t capacity;
    size_t current;
    Time lastRecordEpochTime;
    void clear() {
      current = 0, lastRecordEpochTime = 0;
      history.clear();
      history.reserve(capacity);
    }
  public:
    Recorder(size_t time_interval, size_t capacity) :
      time_interval(time_interval), capacity(capacity) {
      clear();
    }
    void addRecord(Record const& t) {
      if (history.size() <= capacity) {
        history.push_back(t);
        current = history.size();
      } else {
        if (current >= capacity)
          current = 0;
        history[current] = t;
        current++;
      }
    }
    void updateRecord(Record const& t) {
      if (history.empty())
        addRecord(t);
      else
        history[current - 1] = t;
    }

    void record(Record const& t, Time epochTime) {
      String log;
      log = "record(epochTime=";
      log += epochTime;
      log += ") size=";
      log += history.size();
      LOG(log);

      if (epochTime < lastRecordEpochTime) {
        log = "epochTime < lastRecordEpochTime";
        LOG(log);
        clear();
      }
      if (!lastRecordEpochTime)
        lastRecordEpochTime = epochTime;
      unsigned long gap = (epochTime - lastRecordEpochTime) / time_interval;
      if (gap >= capacity) {
        log = "gap >= capacity";
        LOG(log);
        clear();
        gap = 0;
      }
      if (gap) {
        while (gap--)
          addRecord(t);
      }
      updateRecord(t);
      lastRecordEpochTime = epochTime;
    }
  private:
    size_t delta2current(size_t d) {
      return (history.size() + current - d ) % history.size();
    }
  public:
    std::vector<std::pair<Time, Record>> query(Time pastBegin, Time pastEnd, bool(*giveup_func)(void) = nullptr) {
      std::vector<std::pair<Time, Record>> result;
      size_t size = history.size();
      if (size <= 0)
        return result;
      Time deltaPastBegin = 0;
      if (pastBegin < lastRecordEpochTime)
        deltaPastBegin = (lastRecordEpochTime - pastBegin) / time_interval;
      if (deltaPastBegin + 1 >= size)
        deltaPastBegin = size - 1;
      Time deltaPastEnd = size - 1;
      if (pastEnd < lastRecordEpochTime)
        deltaPastEnd = (lastRecordEpochTime - pastEnd) / time_interval;
      if (deltaPastEnd + 1 >= size)
        deltaPastEnd = size - 1;
      if (deltaPastBegin < deltaPastEnd)
        return result;
      String log;
      log += "size=";
      log += size;
      log += " deltaPastBegin=";
      log += deltaPastBegin;
      log += " deltaPastEnd=";
      log += deltaPastEnd;
      log += " lastRecordEpochTime=";
      log += lastRecordEpochTime;
      LOG(log);
      for (Time i = deltaPastBegin; i >= deltaPastEnd; i--) {
        Time record_time = lastRecordEpochTime - (i * time_interval);
        result.push_back(std::make_pair(record_time, history[delta2current(i)]));
        if (giveup_func && (*giveup_func)()) {
          log = "giving up";
          LOG(log);
          break;
        }
      }
      return result;
    }
};

#endif
