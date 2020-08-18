#pragma once
#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unistd.h>

#include <boost/lockfree/queue.hpp>

template <typename Key, typename Value> struct IConsumer {
  virtual void Consume(Key id, const Value &value) {
    id;
    value;
  }
};

template <typename Key, typename Value, size_t BufferSize> struct Subscriber {
  using lockfree_queue =
      boost::lockfree::queue<Value, boost::lockfree::fixed_sized<true>,
                             boost::lockfree::capacity<BufferSize>>;

  Key key_;
  IConsumer<Key, Value> *consumer_ = nullptr;
  lockfree_queue values_;

  bool exit = false;

  Subscriber() = delete;
  Subscriber(const Key& key, IConsumer<Key, Value> *consumer)
      : key_(key), consumer_(consumer){};
  ~Subscriber() { exit = true; }
  Subscriber(const Subscriber &) = delete;
  Subscriber &operator=(const Subscriber &) = delete;
  Subscriber(Subscriber &&rhs) = delete;
  Subscriber &operator=(Subscriber &&rhs) = delete;
  void Add(Value &&value) {
    while (!exit && !values_.push(std::move(value)))
      std::this_thread::sleep_for(std::chrono::nanoseconds(1));
  }
  void Call() {
    Value v;
    int cnt = 0;
    while (values_.pop(v) && cnt++ < 5)
      consumer_->Consume(key_, v);
  }
};

template <typename Key, typename Value, size_t BufferSize = 1000>
class MultiQueueProcessor {
  using lockfree_queue =
      boost::lockfree::queue<Value, boost::lockfree::fixed_sized<true>,
                             boost::lockfree::capacity<BufferSize>>;
  using Consumer = Subscriber<Key, Value, BufferSize>;
  using Consumer_ptr = std::unique_ptr<Consumer>;

public:
  MultiQueueProcessor() {
    worker_ = std::thread(&MultiQueueProcessor::Process, this);
  }
  ~MultiQueueProcessor() {
    exit = true;
    worker_.join();
  }
  void Subscribe(const Key& id, IConsumer<Key, Value> *consumer) {
    std::lock_guard<std::shared_mutex> lock{consumersMtx};
    subscribers.try_emplace(id, std::make_unique<Consumer>(id, consumer));
  }
  void Unsubscribe(const Key& id) {
    std::lock_guard<std::shared_mutex> lock{consumersMtx};
    subscribers.erase(id);
  }
  void Enqueue(const Key& id, Value value) {
    std::shared_lock<std::shared_mutex> lock{consumersMtx};
    auto pos = subscribers.find(id);
    if (pos == subscribers.end())
      return;
    pos->second->Add(std::move(value));
  }
  void Process() {
    while (!exit) {
      std::shared_lock<std::shared_mutex> lock{consumersMtx};
      for (auto &i : subscribers) {
        i.second->Call();
      }
    }
  }

protected:
  std::shared_mutex consumersMtx;
  std::map<Key, Consumer_ptr> subscribers;
  std::thread worker_;
  std::atomic<bool> exit{false};
};