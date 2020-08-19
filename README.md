# Multi Queue Message Processor.

Constant number of threads.
Uses boost::fibers and boost::lockfree::queue.

# Depends
Boost 1.71.0;
C++17;
pthread.

I've used conan as package manager to get boost.

# Usage
#include "include/MultiQueueProcessor.h"

Example:
```
  struct IConsumerClass : IConsumer<std::string, int> {
    void Consume(const std::string& key, const int &value) override { 
      std::cout << "Queue " << key << ": " << value << '\n';
    }
  };
  
  ...
  IConsumerClass IConsumerObj;
  MQProcessor::Queue<std::string,int,512,8,128> mq; // register message queue with std::string as key, and int as data
                                                    // Template Arguments: 
                                                    //         Key Type
                                                    //         Value Type
                                                    //         Size of queue buffer
                                                    //         Threads Count
                                                    //         Max number of consumers
  mq.Subscribe(key, &IConsumerObj );                // Subscribe on queue with key and push data to IConsumer Interface
  mq.Enqueue(key,value);                            // Push value to queue key
  mq.Unsubscribe(key);                              // Unsubscribe from queue  
```

Value supports only trivial assignable and trivial destructible types.

More examples in /test folder.

# Alternative Version:
On branch 1ConsumerNThreads: use N threads per each data consumer, work better with small amount of consumers, but degrade with more consumers, plus create many threads.

Usage:
```
  struct IConsumerClass : IConsumer<std::string, int> {
    void Consume(const std::string& key, const int &value) override { 
      std::cout << "Queue " << key << ": " << value << '\n';
    }
  };
  
  ...
  MQProcessor::Queue<std::string, int> queue(1024,2,64);// register message queue with std::string as key, and int as data.
                                                        // Params: 
                                                        //         Size of queue buffer
                                                        //         Threads Per Consumer
                                                        //         Max number of consumers
  mq.Subscribe(key, &IConsumerObj );      // Subscribe on queue with key and push data to IConsumer Interface
  mq.Enqueue(key,value);                 //push value to queue key
  mq.Unsubscribe(key);                   //Unsubscribe from queue  
```
