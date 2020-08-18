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
// #include<boost/lockfree/queue.hpp>

#include "ThreadFiberPool.hpp"

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

template<typename Key, typename Value>
void Sender( eventFiber& exit, const Key& key, boost::fibers::buffered_channel<Value>& channel, IConsumer<Key, Value>* consumer_ ) {
  Value v;
  while( !exit.WaitForEvent(0) ) {
    size_t count = 0;
    while (channel.pop(v) == boost::fibers::channel_op_status::success) {
      consumer_->Consume(key, v);
      if( ++count == 10 ) break;
    }
  }
};

template <typename Key, typename Value, size_t BufferSize = 512>
class Consumer {
  using Channel = boost::fibers::buffered_channel<Value>;
  Key key;
  Channel channel{BufferSize};
  IConsumer<Key, Value> *consumer_;
  std::thread worker_;
public:
  Consumer(Key k, IConsumer<Key, Value> *c)
      : key(k), consumer_(c){};
  ~Consumer() {
    channel.close();
  }
  void Add(Value &&val) { channel.push(std::move(val)); }
  Channel& GetChannel(){ return channel; };
};

template <typename Key, typename Value, size_t BufferSize = 1024>
class MultiQueueProcessor {
  using Channel = boost::fibers::buffered_channel<Value>;
  using fiberPool_t = ThreadFiberPool::ThreadPool<Key, Channel&, IConsumer<Key, Value>*>;
public:
  MultiQueueProcessor() {
    threadPool = std::make_unique<fiberPool_t>( Sender<Key,Value>, 8 );
  }
  ~MultiQueueProcessor() {
    threadPool.release();
  }
  void Subscribe(const Key &id, IConsumer<Key, Value> *consumer) {
    std::lock_guard<std::shared_mutex> lock{consumersLock};
    auto [data,ok] = consumers.try_emplace(id, std::make_unique<Consumer_t>(id, consumer));
    if( !ok )
      return;
    threadPool->Add( id, (*data).second->GetChannel(), consumer );
  }

  void Unsubscribe(const Key &id) {
    std::lock_guard<std::shared_mutex> lock{consumersLock};
    threadPool->Delete(id);
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

  std::unique_ptr<fiberPool_t> threadPool;
};