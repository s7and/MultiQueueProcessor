#ifndef MULTIQUEUEPROCESSOR_TESTSTRUCTURES
# define MULTIQUEUEPROCESSOR_TESTSTRUCTURES

#pragma once

#include "MultiQueueProcessor.h"
#include <atomic>

 struct TestConsumer : IConsumer<int, int> {
  void Consume( int , const int &) override { consumed++; }
  uint64_t consumed = 0;
};

template<size_t BufferSize, bool dummy = false>
struct Providers {
  std::vector<std::thread> workers;
  std::atomic<uint64_t> sended{0};
  const size_t countThreads;
  MultiQueueProcessor<int, int> &mqproc;
  const int consumerKey;
  Providers(MultiQueueProcessor<int, int> &mq, const size_t count,
            const int consumer)
      : mqproc(mq), countThreads(count), consumerKey(consumer){};
  Providers(const Providers &) = delete;
  Providers &operator=(const Providers &) = delete;
  Providers(Providers &&rhs) noexcept
      : consumerKey(rhs.consumerKey), countThreads(rhs.countThreads),
        sended(rhs.sended.load()), mqproc(rhs.mqproc) {
    std::swap(workers, rhs.workers);
  };
  Providers &operator=(Providers && rhs) {
    *this = std::move(rhs);
    return *this;
  };
  void Start() {
    for (size_t i = 0; i < countThreads; i++) {
      workers.emplace_back(std::thread([this]() {
        auto start = std::chrono::system_clock::now();
        auto now = std::chrono::system_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(now - start)
                   .count() < 1 ) {
          mqproc.Enqueue(consumerKey, 1);
          sended++;
          now = std::chrono::system_clock::now();
          if( dummy)
            std::this_thread::sleep_for( std::chrono::seconds( 1 ));
        }
      }));
    };
  }
  void Join() {
    for (auto &i : workers) {
      i.join();
    }
  }
  uint64_t Sended() const { return sended.load(); }
};

#endif