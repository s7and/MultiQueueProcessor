#pragma once
#include <atomic>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>


template <typename Key, typename Value> struct IConsumer {
  virtual void Consume(Key id, const Value &value) {
    id;
    value;
  }
};

template <typename Key, typename Value> class Subscriber {
  using consumer_t = IConsumer<Key, Value>*;
  const size_t BufferSize = 1024;
  const size_t ThreadCount = 4;
  const size_t bufferDivider = ThreadCount == 1 ? 2 : ThreadCount;

  std::mutex lk_;
  std::condition_variable dataCv_;
  std::condition_variable addAvail_;

  const Key key_;
  const consumer_t consumer_ = nullptr;
  std::atomic<bool> exit{false};
  std::vector<std::thread> workers_;

  std::list<Value> values_;
public:
  Subscriber(const Key& key, const consumer_t consumer, const size_t bufferSize, const size_t threadCount)
      : BufferSize(bufferSize), ThreadCount(threadCount), key_(key), consumer_(consumer),
        bufferDivider(ThreadCount == 1 ? 2 : ThreadCount) {
    for( auto i = 0; i < ThreadCount; i++ )
      workers_.emplace_back( std::thread(&Subscriber::Process, this) );
    addAvail_.notify_one();
  };
  ~Subscriber() {
    exit = true;
    for( auto& i : workers_ )
      i.join();
  }
  Subscriber(const Subscriber &) = delete;
  Subscriber &operator=(const Subscriber &) = delete;
  Subscriber(Subscriber &&rhs) = delete;
  Subscriber &operator=(Subscriber &&rhs) = delete;
  void Add(Value &&value) {
    std::unique_lock<std::mutex> lk(lk_);
    while (!exit && values_.size() == BufferSize) {
      if (addAvail_.wait_for(lk, std::chrono::microseconds(1)) ==
          std::cv_status::no_timeout)
        break;
    }
    if (exit)
      return;
    values_.emplace_back(std::move(value));
    if (values_.size() > BufferSize / bufferDivider)
      dataCv_.notify_one();
  }
  void SetExit() {
    exit = true;
  }
  bool Get(std::list<Value> &cp) {
    std::unique_lock<std::mutex> lk(lk_);
    while (!exit && values_.size() == 0) {
      if (dataCv_.wait_for(lk, std::chrono::microseconds(1)) ==
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
      for (auto &i : cp) {
        consumer_->Consume(key_, i);
        if( exit ) 
          break;
      }
      cp.clear();
    }
  }
};

template <typename Key, typename Value>
class MultiQueueProcessor {
  using consumer_t = IConsumer<Key, Value>*;
  using subscriber_t = Subscriber<Key, Value>;
  using subscriber_ptr = std::unique_ptr<subscriber_t>;
public:
  MultiQueueProcessor( const size_t bufferSize = 1024, const size_t threadCount = 8, const size_t maxSubscribers = 64 ) 
    : BufferSize(bufferSize), ThreadCount(threadCount), MaxSubscribers(maxSubscribers) {
      if( bufferSize == 0 )
        throw std::runtime_error("MultiQueueProcessor Wrong Arguments");
      if( threadCount == 0 )
        throw std::runtime_error("MultiQueueProcessor Wrong Arguments");
      if( maxSubscribers == 0 || maxSubscribers > 64 )
        throw std::runtime_error("MultiQueueProcessor Wrong Arguments");
    };
  MultiQueueProcessor( const MultiQueueProcessor&) = delete;
  MultiQueueProcessor& operator=( const MultiQueueProcessor&) = delete;
  MultiQueueProcessor(MultiQueueProcessor&&) = default;
  MultiQueueProcessor& operator=(MultiQueueProcessor&&) = default;
  ~MultiQueueProcessor() {
    exit = true;
    for( auto& i : subscribers )
      i.second->SetExit();
  }
  bool Subscribe(const Key& id, const consumer_t consumer) {
    if( exit )
      return false;
    std::lock_guard<std::shared_mutex> lock{consumersMtx};
    if( MaxSubscribers <= subscribers.size()  )
      return false;
    auto [iter, ok] = subscribers.try_emplace(id, std::make_unique<subscriber_t>(id, consumer, BufferSize, ThreadCount));
    return ok;
  }
  bool Unsubscribe(const Key& id) {
    if( exit )
      return false;
    std::lock_guard<std::shared_mutex> lock{consumersMtx};
    return subscribers.erase(id);
  }
  bool Enqueue(const Key& id, Value value) {
    if( exit )
      return false;
    std::shared_lock<std::shared_mutex> lock{consumersMtx};
    auto pos = subscribers.find(id);
    if (pos == subscribers.end())
      return false;
    pos->second->Add(std::move(value));
    return true;
  }

private:
  const size_t BufferSize = 1024;
  const size_t ThreadCount = 2;
  const size_t MaxSubscribers = 64;
  std::atomic<bool> exit{false};
  std::shared_mutex consumersMtx;
  std::map<Key, subscriber_ptr> subscribers;
};