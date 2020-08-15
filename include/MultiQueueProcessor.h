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
# define Sleep usleep
#endif

#define MaxCapacity 1000

template <typename Key, typename Value, size_t BufferSize = 512> class MultiQueueProcessor {
public:
  MultiQueueProcessor() : worker_( &MultiQueueProcessor::Process, this ) {}

  ~MultiQueueProcessor() {
    exit = true;
    worker_.join();
    consumers.clear();
  }

  void Subscribe(const Key& id, IConsumer<Key, Value> *consumer) {
    std::lock_guard<std::shared_mutex> lock{consumersLock};
    auto iter = consumers.find(id);
    if (iter == consumers.end()) {
      consumers.emplace(std::make_pair( id, 
                        consumer_ptr(new Consumer(id, consumer))));
    }
  }

  void Unsubscribe(const Key& id) {
    std::lock_guard<std::shared_mutex> lock{consumersLock};
    auto iter = consumers.find(id);
    if (iter != consumers.end())
      consumers.erase(id);
  }

  void Enqueue(const Key& id, Value value) {
    std::shared_lock<std::shared_mutex> lock{consumersLock};
    auto iter = consumers.find(id);
    if (iter == consumers.end())
      return;
    iter->second->Add(std::move(value));
  }

protected:
  void Process() {
    while (!exit) {
      std::shared_lock<std::shared_mutex> lock{consumersLock};
      for (auto &i : consumers)
        i.second->Call();
    }
  }

protected:
  using Channel = boost::fibers::buffered_channel<Value>;
  struct Consumer {
    Key key;
    Channel channel{BufferSize};
    IConsumer<Key, Value> *consumer_;
    Consumer(Key k, IConsumer<Key, Value> *c) : key(k), consumer_(c) {};
    ~Consumer() {
      channel.close();
    }
    Consumer(Consumer &&rhs) = delete;
    void Add(Value &&val) {
      channel.push(std::move(val));
    }
    void Call() {
      Value v;
      while (!channel.is_closed() && channel.try_pop(v) == boost::fibers::channel_op_status::success) {
        consumer_->Consume(key, v);
      }
    }
  };
  using consumer_ptr = std::unique_ptr<Consumer>;
  std::map<Key, consumer_ptr> consumers;
  std::shared_mutex consumersLock;

  std::atomic<bool> exit{false};
  std::thread worker_;
};