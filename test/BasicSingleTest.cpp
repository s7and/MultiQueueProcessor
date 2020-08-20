#include "MultiQueueProcessor.h"

#include <iostream>
#include <vector>

struct TestConsumer : IConsumer<int, int> {
  void Consume(const int &id, const int &value) override {
    (void)id;
    consumed.push_back(value);
  }
  std::vector<int> consumed;
};

int main() {
  TestConsumer tc;
  {
    MQProcessor::Queue<int, int> queue;
    queue.Subscribe(1, &tc);
    for (int i = 0; i < 10; i++) {
      queue.Enqueue(1, i);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
  return tc.consumed != std::vector{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
}