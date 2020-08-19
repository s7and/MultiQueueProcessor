#include "MultiQueueProcessor.h"
#include "TestStructures.hpp"
#include <chrono>
#include <thread>
#include <vector>

int main(int , char **) {
  const size_t countCons = 16;
  const size_t countThreads = 16;
  const size_t BufferSize = 512;
  MultiQueueProcessor<int, int> mqproc;
  std::vector<TestConsumer> consumers(countCons, TestConsumer());
  std::vector<Providers<BufferSize>> providers;
  for (int i = 0; i < countCons; i++)
    mqproc.Subscribe(i, &consumers[i]);
  for (int i = 0; i < countCons; i++)
    providers.emplace_back(Providers<BufferSize>(mqproc, countThreads, i));
  if( !mqproc.Unsubscribe(countCons / 2) )
    return 1;
  if( !mqproc.Unsubscribe(countCons / 4) )
    return 2;
  if( !mqproc.Subscribe(countCons / 2, &consumers[countCons / 2]) )
    return 3;
  if( !mqproc.Unsubscribe(countCons / 8) )
    return 4;
  if( mqproc.Unsubscribe(countCons + 1) )
    return 5;
  if( mqproc.Subscribe(0, &consumers[0]) )
    return 6;
  for (auto &i : providers)
    i.Start();
  for (auto &i : providers)
    i.Join();
  return 0;
}