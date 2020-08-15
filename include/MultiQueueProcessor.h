#pragma once
#include <atomic>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unistd.h>

#include <boost/fiber/buffered_channel.hpp>

template <typename Key, typename Value> struct IConsumer {
  virtual void Consume(Key id, const Value &value) {
    id;
    value;
  }
};

#ifndef Sleep
#define Sleep usleep
#endif

#define MaxCapacity 1000

template <typename Key, typename Value, size_t BufferSize = 512>
class Consumer {
  using Channel = boost::fibers::buffered_channel<Value>;
  Key key;
  Channel channel{BufferSize};
  IConsumer<Key, Value> *consumer_;
  std::thread worker_;
  void Process() {
    Value v;
    while (!channel.is_closed()) {
      if (channel.pop(v) != boost::fibers::channel_op_status::success)
        continue;
      consumer_->Consume(key, v);
    }
  }
public:
  Consumer(Key k, IConsumer<Key, Value> *c)
      : key(k), consumer_(c), worker_(&Consumer::Process, this){};
  ~Consumer() {
    channel.close();
    worker_.join();
  }
  void Add(Value &&val) { channel.push(std::move(val)); }
};

template <typename Key, typename Value, size_t BufferSize = 512>
class MultiQueueProcessor {
public:
  void Subscribe(const Key &id, IConsumer<Key, Value> *consumer) {
    std::lock_guard<std::shared_mutex> lock{consumersLock};
    consumers.try_emplace(id, std::make_unique<Consumer_t>(id, consumer));
  }

  void Unsubscribe(const Key &id) {
    std::lock_guard<std::shared_mutex> lock{consumersLock};
    consumers.erase(id);
  }

  void Enqueue(const Key &id, Value value) {
    std::shared_lock<std::shared_mutex> lock{consumersLock};
    auto iter = consumers.find(id);
    if (iter == consumers.end())
      return;
    iter->second->Add(std::move(value));
  }

private:
  using Consumer_t = Consumer<Key, Value, BufferSize>;
  using consumer_ptr = std::unique_ptr<Consumer_t>;

  std::map<Key, consumer_ptr> consumers;
  std::shared_mutex consumersLock;
};