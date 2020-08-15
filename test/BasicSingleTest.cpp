#include "MultiQueueProcessor.h"

#include <vector>
#include <iostream>

struct TestConsumer : IConsumer<int,int> {
    void Consume( int id, const int& value ) override {
        consumed.push_back(value);
    }
    std::vector<int> consumed;
};

int main()
{
    MultiQueueProcessor<int,int> queue;
    TestConsumer tc;
    queue.Subscribe(1,&tc);
    for( int i = 0; i < 10; i++ ) {
        queue.Enqueue( 1, i );
    }
    Sleep(5000);
    return tc.consumed != std::vector{1,2,3,4,5,6,7,8,9};
}