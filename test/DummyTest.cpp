#include "MultiQueueProcessor.h"
#include "TestStructures.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>



int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Wrong Args!\n";
    return 1;
  }
  const size_t countCons = atoi(argv[1]);
  if (countCons > 10000) {
    std::cout << "Wrong countCons!\n";
    return 1;
  }
  const size_t countThreads = atoi(argv[2]);
  if (countThreads > 50) {
    std::cout << "Wrong countThreads!\n";
    return 1;
  }
  const size_t BufferSize = 512;
  MQProcessor::Queue<int, int> mqproc;
  std::vector<TestConsumer> consumers(countCons, TestConsumer());
  std::vector<Providers<BufferSize,true>> providers;
  for (int i = 0; i < countCons; i++)
    mqproc.Subscribe(i, &consumers[i]);
  return 0;
}