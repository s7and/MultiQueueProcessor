#ifndef FIBERSBALANCER
# define FIBERSBALANCER

# pragma once

#include <vector>
#include <atomic>
#include <chrono>
#include <thread>
#include <memory>
#include <list>
#include <tuple>
#include <boost/fiber/all.hpp>
#include <boost/system/config.hpp>

#include <mutex>
#include <condition_variable>
#include "event.hpp"

using eventFiber = event<void,false,boost::fibers::mutex,boost::fibers::condition_variable>;
using eventThread = event<void,false,std::mutex,std::condition_variable>;

namespace ThreadFiberPool {
  template<typename Function, typename Id, typename... Params>
  class Worker {
    Id id_;
    Function* func;
    boost::fibers::fiber wrk;
    eventFiber exit;
    std::tuple<eventFiber&, const Id&, Params...> data;
  public:
    Worker( Function* fn, const Id& id, Params... params) : 
      func(fn), id_(id), data(std::make_tuple( std::ref(exit), std::ref(id), std::ref(params)... ) ) {}
    Worker(Worker&&) = default;
    Worker& operator=(Worker&&) = default;
    void Start() {
      wrk = std::move( boost::fibers::fiber([&]() {
          std::apply( func, data );
          }) );
        boost::this_fiber::yield();
    }
    const Id& id() const { return id_; };
    ~Worker() {
      exit.SetEvent();
      wrk.join();
    }
  };

  template<typename Function, typename Id, typename... Args>
  class FiberPool {
    using worker_type = Worker<Function,Id,Args...>;
    using worker_ptr = std::unique_ptr<worker_type>;
    Function* func;
    std::thread workThread;
    std::vector<worker_ptr> pool;
    std::list<worker_ptr> waitList;

    eventThread exit;
    eventThread added;

    void Routine() {
      boost::fibers::use_scheduling_algorithm< boost::fibers::algo::shared_work >();
      while(!exit.WaitForEvent(0)) {
        while( waitList.size() > 0 ) {
          worker_ptr newWorker = std::move( waitList.back() );
          waitList.pop_back();
          newWorker->Start();
          pool.emplace_back( std::move( newWorker ) );
          added.SetEvent();
        }
        boost::this_fiber::sleep_for(std::chrono::milliseconds(10));
        // boost::this_fiber::yield();
      }
    }
  public:
    FiberPool( Function* fn ) : func(fn) {
      workThread = std::thread( &FiberPool::Routine, this );
    } ;
    ~FiberPool() {
      exit.SetEvent();
      pool.clear();
      workThread.join();
    }
    FiberPool(const FiberPool&) = delete;
    FiberPool& operator=(const FiberPool&) = delete;
    FiberPool(FiberPool&&) = default;
    FiberPool& operator=(FiberPool&&) = default;
    void Add( const Id& id, Args... args) {
      waitList.emplace_back( std::move( worker_ptr( new worker_type( func, id, args...) )));
      while( !added.WaitForEvent(0) )
        std::this_thread::sleep_for( std::chrono::nanoseconds(0));
    }
    void Delete(const Id& id) {
      auto elem = std::find_if( pool.begin(), pool.end(), [id]( const worker_ptr& lhs ){
        return lhs->id() == id;
      });
      if( elem == pool.end() )
        return;
      pool.erase( elem );
    }
    size_t size() const { return pool.size(); };
  };

  template<typename Id, typename... Args>
  class ThreadPool {
    using Function = void( eventFiber&, const Id&, Args... );
    using fiber_type = FiberPool<Function,Id,Args...>;
    using fiber_ptr = std::unique_ptr<fiber_type>;
    std::vector<fiber_ptr> pool;
    Function* func;
  public:
    ThreadPool( Function* fn, size_t threadCount = 4 ) : func(fn) {
      for( auto i = 0; i < threadCount; i++ )
        pool.emplace_back( std::move( fiber_ptr( new fiber_type( func ) ) ) );
    };
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = default;
    ThreadPool& operator=(ThreadPool&&) = default;
    void Add( const Id& id, Args... args ) {
      auto min = std::min_element( pool.begin(), pool.end(), []( const fiber_ptr& lhs, const fiber_ptr& rhs ){
        return lhs->size() < rhs->size();
      } );
      min->get()->Add( id, args... );
    }
    void Delete( const Id& id ) {
      for( auto& i : pool )
        i->Delete(id);
    }
  };

}

#endif