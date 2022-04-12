#ifndef RECORDER_HH
#define RECORDER_HH
#include <vector>
#include <TimeLib.h>

template<class Time, class Record> class Recorder {
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
      if (epochTime < lastRecordEpochTime)
        clear();
      if (!lastRecordEpochTime)
        lastRecordEpochTime = epochTime;
      unsigned long gap = (epochTime - lastRecordEpochTime) / time_interval;
      if (gap >= capacity) {
        clear();
        gap = 0;
      }
      if (gap) {
        while (gap--)
          addRecord (Record());
        lastRecordEpochTime = epochTime;
      }
      updateRecord(t);
    }
  private:
    size_t delta2current(size_t d) {
      return (history.size() + current - d ) % history.size();
    }
  public:
    std::vector<std::pair<Time, Record>> query(Time begin, Time end, bool(*giveup_func)(void) = nullptr) {
      std::vector<std::pair<Time, Record>> result;
      size_t size = history.size();
      long deltaBegin = (lastRecordEpochTime - begin) / time_interval;
      if (deltaBegin < 0)
        deltaBegin = 0;
      if (deltaBegin + 1 >= size)
        deltaBegin = size - 1;
      long deltaEnd = (lastRecordEpochTime - end) / time_interval;
      if (deltaEnd < 0)
        deltaEnd = 0;
      if (deltaEnd + 1 >= size)
        deltaEnd = size - 1;
      if (deltaBegin < deltaEnd)
        return result;
      for (; deltaBegin >= deltaEnd; deltaBegin--, begin += time_interval) {
        result.push_back(std::make_pair(begin, history[delta2current]));
        if (giveup_func && (*giveup_func)())
          break;
      }
      return result;
    }
};

#endif
