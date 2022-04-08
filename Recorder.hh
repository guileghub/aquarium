#ifndef RECORDER_HH
#define RECORDER_HH
#include <vector>
#include <TimeLib.h>

template<class T> class Recorder {
    std::vector<T> history;
    size_t time_interval;
    size_t capacity;
    size_t current;
    time_t lastRecordEpochTime;
    void clear() {
      current = 0, lastRecordEpochTime = 0;
      history.clear();
    }
  public:
    Recorder(size_t time_interval, size_t capacity) :
      time_interval(time_interval), capacity(capacity) {
      clear();
    }
    void addRecord(T const& t) {
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
    void updateRecord(T const& t) {
      if (history.empty())
        addRecord(t);
      else
        history[current - 1] = t;
    }

    void record(T const& t, time_t epochTime) {
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
          addRecord (T());
        lastRecordEpochTime = epochTime;
      }
      updateRecord(t);
    }
    std::vector<T> query(time_t begin, time_t end) {
      std::vector < T > result;
      size_t size = history.size();
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
        result.push_back(history[i]);
        if (i >= size)
          i = 0;
      }
      return result;
    }
};

#endif
