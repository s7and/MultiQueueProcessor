#ifndef MULTIQUEUEPROCESSOR_TESTSTRUCTURES
#define MULTIQUEUEPROCESSOR_TESTSTRUCTURES

#pragma once

#include "MultiQueueProcessor.h"
#include <atomic>

struct TestConsumer : IConsumer<int, int> {
  void Consume(const int &, const int &) override { consumed++; }
  uint64_t consumed = 0;
};

template <size_t BufferSize, bool dummy = false> struct Providers {
  MQProcessor::Queue<int, int> &mqproc;
  const size_t countThreads;
  const int consumerKey;
  std::vector<std::thread> workers;
  std::atomic<uint64_t> sended{0};
  Providers(MQProcessor::Queue<int, int> &mq, const size_t count,
            const int consumer)
      : mqproc(mq), countThreads(count), consumerKey(consumer){};
  Providers(const Providers &) = delete;
  Providers &operator=(const Providers &) = delete;
  Providers(Providers &&rhs) noexcept
      : mqproc(rhs.mqproc), countThreads(rhs.countThreads),
        consumerKey(rhs.consumerKey), sended(rhs.sended.load()) {
    std::swap(workers, rhs.workers);
  };
  Providers &operator=(Providers &&rhs) {
    *this = std::move(rhs);
    return *this;
  };
  void Start() {
    for (size_t i = 0; i < countThreads; i++) {
      workers.emplace_back(std::thread([this]() {
        auto start = std::chrono::system_clock::now();
        auto now = std::chrono::system_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(now - start)
                   .count() < 1) {
          mqproc.Enqueue(consumerKey, 1);
          sended++;
          now = std::chrono::system_clock::now();
          if (dummy)
            std::this_thread::sleep_for(std::chrono::seconds(1));
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