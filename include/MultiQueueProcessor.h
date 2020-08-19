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

template <typename Key, typename Value, size_t BufferSize, size_t ThreadCount> class Subscriber {
  static constexpr size_t bufferDevider = ThreadCount == 1 ? 2 : ThreadCount;
  using consumer_t = IConsumer<Key, Value>*;

  std::mutex lk_;
  std::condition_variable dataCv_;
  std::condition_variable addAvail_;

  Key key_;
  consumer_t consumer_ = nullptr;
  std::atomic<bool> exit{false};
  std::vector<std::thread> workers_;

  std::list<Value> values_;
public:
  Subscriber(const Key& key, const consumer_t consumer)
      : key_(key), consumer_(consumer) {
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
    if (values_.size() > BufferSize / bufferDevider)
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

template <typename Key, typename Value, size_t BufferSize = 1024, size_t ThreadCount = 4>
class MultiQueueProcessor {
  using consumer_t = IConsumer<Key, Value>*;
  using subscriber_t = Subscriber<Key, Value, BufferSize, ThreadCount>;
  using subscriber_ptr = std::unique_ptr<subscriber_t>;
public:
  MultiQueueProcessor() = default;
  MultiQueueProcessor( const MultiQueueProcessor&) = delete;
  MultiQueueProcessor& operator=( const MultiQueueProcessor&) = delete;
  MultiQueueProcessor(MultiQueueProcessor&&) = default;
  MultiQueueProcessor& operator=(MultiQueueProcessor&&) = default;
  ~MultiQueueProcessor() {
    exit = true;
    for( auto& i : subscribers )
      i.second->SetExit();
  }
  void Subscribe(const Key& id, const consumer_t consumer) {
    if( exit )
      return;
    std::lock_guard<std::shared_mutex> lock{consumersMtx};
    subscribers.try_emplace(id, std::make_unique<subscriber_t>(id, consumer));
  }
  void Unsubscribe(const Key& id) {
    if( exit )
      return;
    std::lock_guard<std::shared_mutex> lock{consumersMtx};
    subscribers.erase(id);
  }
  void Enqueue(const Key& id, Value value) {
    if( exit )
      return;
    std::shared_lock<std::shared_mutex> lock{consumersMtx};
    auto pos = subscribers.find(id);
    if (pos == subscribers.end())
      return;
    pos->second->Add(std::move(value));
  }

private:
  std::atomic<bool> exit{false};
  std::shared_mutex consumersMtx;
  std::map<Key, subscriber_ptr> subscribers;
};