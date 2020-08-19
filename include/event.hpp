#ifndef EVENTHPP
#define EVENTHPP

#pragma once

template <typename Data = void, bool AutoReset = false,
          typename MutexType = std::mutex,
          typename CVType = std::condition_variable>
class event {
  MutexType _cv_lock;
  CVType _cv_eventUpdated;
  bool eventCame = false;
  Data data;

public:
  std::pair<bool, Data> WaitForEvent(uint64_t timeout) {
    std::unique_lock<MutexType> _lk(_cv_lock);
    bool status = _cv_eventUpdated.wait_for(
        _lk, std::chrono::nanoseconds(timeout), [this] { return eventCame; });
    if (status && AutoReset)
      eventCame = false;
    return std::pair<bool, Data>{status, data};
  };
  void SetEvent(Data &&d) {
    std::unique_lock<MutexType> _lk(_cv_lock);
    eventCame = true;
    std::swap(data, d);
    _cv_eventUpdated.notify_all();
  }
};

template <bool AutoReset, typename MutexType, typename CVType>
class event<void, AutoReset, MutexType, CVType> {
  MutexType _cv_lock;
  CVType _cv_eventUpdated;
  bool eventCame = false;

public:
  bool WaitForEvent(uint64_t timeout) {
    std::unique_lock<MutexType> _lk(_cv_lock);
    bool status = _cv_eventUpdated.wait_for(
        _lk, std::chrono::nanoseconds(timeout), [this] { return eventCame; });
    if (status && AutoReset)
      eventCame = false;
    return status;
  }
  void SetEvent() {
    std::unique_lock<MutexType> _lk(_cv_lock);
    eventCame = true;
    _cv_eventUpdated.notify_all();
  }
};

// using eventExit = event<void,false>;

#endif