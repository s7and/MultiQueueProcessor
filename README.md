# MultiQueueProcessor

Uses conditional_variables and threads.

N threads per one data consumer.

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
