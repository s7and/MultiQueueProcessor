#ifndef MULTIQUEUEPROCESSOR
# define MULTIQUEUEPROCESSOR

#pragma once
#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include "ThreadFiberPool.hpp"
#include <boost/lockfree/queue.hpp>

template <typename Key, typename Value> struct IConsumer {
  virtual void Consume(const Key& id, const Value& value) {
    id;
    value;
  }
};

namespace MQProcessor {

  template <typename Key, typename Value, size_t BufferSize> struct Subscriber {
    using lockfree_queue =
        boost::lockfree::queue<Value, boost::lockfree::fixed_sized<true>,
                              boost::lockfree::capacity<BufferSize>>;

    Key key_;
    IConsumer<Key, Value> *consumer_ = nullptr;
    lockfree_queue values_;

    bool exit = false;

    Subscriber() = delete;
    Subscriber(const Key &key, IConsumer<Key, Value> *consumer)
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

  template <typename Key, typename Value, size_t BufferSize>
  void Process(
      eventFiber &exit, const Key &key,
      boost::lockfree::queue<Value, boost::lockfree::fixed_sized<true>,
                            boost::lockfree::capacity<BufferSize>> &queue,
      IConsumer<Key, Value> *consumer) {
    while (!exit.WaitForEvent(1)) {
      int cnt = 0;
      Value v;
      while (queue.pop(v) && cnt++ < BufferSize / 10)
        consumer->Consume(key, v);
    }
  }

  template <typename Key, typename Value, size_t BufferSize = 512,
            size_t ThreadCount = 8, size_t MaxQueues = 128>
  class Queue {
    using lockfree_queue =
        boost::lockfree::queue<Value, boost::lockfree::fixed_sized<true>,
                              boost::lockfree::capacity<BufferSize>>;
    using Consumer = Subscriber<Key, Value, BufferSize>;
    using Consumer_ptr = std::unique_ptr<Consumer>;
    using threadPool_t = ThreadFiberPool::ThreadPool<Key, lockfree_queue &,
                                                    IConsumer<Key, Value> *>;

  public:
    Queue() {
      static_assert( ThreadCount > 0, "Wrong Thread Count" );
      static_assert( BufferSize > 0, "Wrong Buffer Size" );
    };
    Queue(const Queue &) = delete;
    Queue &operator=(const Queue &) = delete;
    Queue(Queue &&) = default;
    Queue &operator=(Queue &&) = default;
    ~Queue() {
      exit = true;
    };
    bool Subscribe(const Key &id, IConsumer<Key, Value> *consumer) {
      if( exit )
        return false;
      std::lock_guard<std::shared_mutex> lock{consumersMtx};
      if (MaxQueues == subscribers.size())
        return false;
      auto [iter, ok] =
          subscribers.try_emplace(id, std::make_unique<Consumer>(id, consumer));
      if (!ok)
        return false;
      threadPool.Add(id, (*iter).second->values_, consumer);
      return true;
    }
    bool Unsubscribe(const Key &id) {
      if( exit )
        return false;
      std::lock_guard<std::shared_mutex> lock{consumersMtx};
      if( threadPool.Delete(id) ) {
        subscribers.erase(id);
        return true;
      }
      return false;
    }
    void Enqueue(const Key &id, Value value) {
      if( exit )
        return;
      std::shared_lock<std::shared_mutex> lock{consumersMtx};
      auto pos = subscribers.find(id);
      if (pos == subscribers.end())
        return;
      pos->second->Add(std::move(value));
    }

  protected:
    std::atomic<bool> exit{false};
    std::shared_mutex consumersMtx;
    std::map<Key, Consumer_ptr> subscribers;
    threadPool_t threadPool{Process<Key, Value, BufferSize>, ThreadCount};
  };

}

#endif