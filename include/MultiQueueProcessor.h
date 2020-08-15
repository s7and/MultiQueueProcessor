#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <shared_mutex>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#ifndef Sleep
#define Sleep(x) usleep(x)
#endif

template <typename Key, typename Value> struct IConsumer {
  virtual void Consume(Key id, const Value &value) {
    id;
    value;
  }
};

template <typename Key, typename Value, size_t BufferSize> struct Subscriber {
  std::mutex lk_;
  std::condition_variable dataCv_;
  std::condition_variable addAvail_;

  Key key_;
  IConsumer<Key, Value> *consumer_ = nullptr;
  std::list<Value> values_;

  std::thread worker_;

  bool exit = false;

  Subscriber() = delete;
  Subscriber(Key key, IConsumer<Key, Value> *consumer)
      : key_(key), consumer_(consumer) {
    addAvail_.notify_one();
    worker_ = std::thread(&Subscriber::Process, this);
  };
  ~Subscriber() {
    exit = true;
    worker_.join();
  }
  Subscriber(const Subscriber &) = delete;
  Subscriber &operator=(const Subscriber &) = delete;
  Subscriber(Subscriber &&rhs) = delete;
  Subscriber &operator=(Subscriber &&rhs) = delete;
  void Add(Value &&value) {
    std::unique_lock<std::mutex> lk(lk_);
    while (values_.size() == BufferSize) {
      if (addAvail_.wait_for(lk, std::chrono::microseconds(1)) ==
          std::cv_status::no_timeout)
        break;
    }
    if (exit)
      return;
    values_.emplace_back(std::move(value));
    if (values_.size() > BufferSize / 2)
      dataCv_.notify_one();
  }
  bool Get(std::list<Value> &cp) {
    std::unique_lock<std::mutex> lk(lk_);
    while (!exit && values_.size() == 0) {
      if (dataCv_.wait_for(lk, std::chrono::microseconds(10)) ==
          std::cv_status::no_timeout)
        break;
    }
    if (exit)
      return false;
    std::swap(cp, values_);
    addAvail_.notify_one();
    return true;
  }
  void Process() {
    std::list<Value> cp;
    while (Get(cp)) {
      for (auto &i : cp)
        consumer_->Consume(key_, i);
      cp.clear();
    }
  }
};

template <typename Key, typename Value, size_t BufferSize = 1000>
class MultiQueueProcessor {
  using Consumer = Subscriber<Key, Value, BufferSize>;
  using Consumer_ptr = std::unique_ptr<Consumer>;

public:
  MultiQueueProcessor() {}
  ~MultiQueueProcessor() { subscribers.clear(); }
  void Subscribe(Key id, IConsumer<Key, Value> *consumer) {
    std::lock_guard<std::shared_mutex> lock{consumersMtx};
    subscribers.try_emplace(id, std::make_unique<Consumer>(id, consumer));
  }
  void Unsubscribe(Key id) {
    std::lock_guard<std::shared_mutex> lock{consumersMtx};
    subscribers.erase(id);
  }
  void Enqueue(Key id, Value value) {
    std::shared_lock<std::shared_mutex> lock{consumersMtx};
    auto pos = subscribers.find(id);
    if (pos == subscribers.end())
      return;
    pos->second->Add(std::move(value));
  }

protected:
  std::shared_mutex consumersMtx;
  std::map<Key, Consumer_ptr> subscribers;
  std::thread worker_;
};