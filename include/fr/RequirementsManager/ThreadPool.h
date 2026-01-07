/**
 * Copyright 2025 Bruce Ide
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <condition_variable>
#include <list>
#include <thread>
#include <memory>
#include <mutex>
#include <fr/RequirementsManager/TaskNode.h>

namespace fr::RequirementsManager {

  class WorkerThread;
  
  /**
   * ThreadState can be used to see what the threadpool is doing
   *
   * Each worker and the Threadpool itself maintain a thread state
   * that can be queried.
   */
  
  enum class ThreadState {
    Starting,
    Ready,
    Processing,
    Draining,
    Shutdown
  };

  // Theadpool for handling database requests. Submit TaskNodes for threadpool
  // to process.
  //
  template <typename WorkerThreadType>
  class ThreadPool : public std::enable_shared_from_this<ThreadPool<WorkerThreadType>> {
    using ThreadPtr = std::shared_ptr<std::thread>;

    // Used to block threads until work is available. This will
    // be passed to worker threads.
    std::shared_ptr<std::mutex> _conditionMutex;
    // Used to block access to the work mutex
    std::shared_ptr<std::mutex> _workMutex;
    // Used to wait in conjuction with conditionMutex.
    // This will be passed to worker threads.
    std::shared_ptr<std::condition_variable> _workCondition;
    // Indicates shutdown has been requested. This puts the
    // thread pool in drain state, where it will not accept
    // more tasks to process. Once all tasks have been drained,
    // the threadpool will terminate its threads and complete
    // shutdown.
    std::atomic<bool> _shutdown;
    // General threadpool state
    std::atomic<ThreadState> _state;
    // Storage for threads
    std::vector<typename WorkerThreadType::PtrType> _threads;
    // Storage for tasks
    std::list<std::shared_ptr<TaskNode<WorkerThreadType>>> _work;

  public:

    ThreadPool() : _shutdown(false), _state(ThreadState::Starting) {
      _conditionMutex = std::make_shared<std::mutex>();
      _workMutex = std::make_shared<std::mutex>();
      _workCondition = std::make_shared<std::condition_variable>();
      _state = ThreadState::Ready;
    }    

    ~ThreadPool() {
      // Make sure we're shut down.
      if (_state != ThreadState::Draining && _state != ThreadState::Shutdown) {
        shutdown();
      }
      // Make sure all threads join before exiting -- this is safe
      // to call multiple times.
      join();
    }

    ThreadState status() {
      return _state;
    }

    void startThreads(unsigned int nthreads) {
      for (int i = 0; i < nthreads; ++i) {
        auto worker = std::make_shared<WorkerThreadType>(this->shared_from_this(), _conditionMutex, _workCondition);
        _threads.push_back(worker);
      }      
    }
    
    std::vector<ThreadState> workerStatus() {
      std::vector<ThreadState> ret;
      for (auto worker : _threads) {
        ret.push_back(worker->status());
      }
      return ret;
    }

    // Returns true if there is work in the work queue
    bool hasWork() {
      std::lock_guard<std::mutex> lock(*_workMutex);
      return (_work.size() > 0);
    }
    
    // Add a task to the work list -- Will init the task if it's not already
    // initted.
    void enqueue(std::shared_ptr<TaskNode<WorkerThreadType>> task) {
      if (!task->initted) {
        task->init();
      }
      task->setOwner(this->shared_from_this());
      {
        std::lock_guard<std::mutex> lock(*_workMutex);
        _work.push_back(task);
      }
      _workCondition->notify_one();
    }

    // Can return nullptr -- threads must check before running task.
    
    std::shared_ptr<TaskNode<WorkerThreadType>> requestWork() {
      std::shared_ptr<TaskNode<WorkerThreadType>> ret;
      std::lock_guard<std::mutex> lock(*_workMutex);
      if (_work.size() > 0) {
        ret = _work.front();
        _work.pop_front();
      }
      return ret;
    }

    // Shut down all the threads -- This just sets a flag requesting
    // shutdown. Join should then be used to block until all threads
    // terminate

    void shutdown() {
      for (auto worker : _threads) {
        worker->shutdown();
      }
      _workCondition->notify_all();
      _state = ThreadState::Draining;
    }

    void join() {
      for (auto worker : _threads) {
        if (worker->joinable()) {
          worker->join();
        }
      }
      _state = ThreadState::Shutdown;
    }
    
  };

  /**
   * This is one thread in a threadpool. It will query the thread pool for
   * tasks and execute any tasks it's given. If there are no current tasks,
   * it will go to sleep with a condition variable passed in from the owning
   * thread pool until the condition variable tells it to wake up.
   */

  class WorkerThread {
  public:
    using Type = WorkerThread;
    using OwnerType = ThreadPool<WorkerThread>;
    using PtrType = std::shared_ptr<Type>;

  private:

    std::atomic<bool> _shutdown;
    std::shared_ptr<OwnerType> _owner;
    std::shared_ptr<std::mutex> _mtx;
    std::shared_ptr<std::condition_variable> _cv;
    std::mutex waitMutex;
    ThreadState _state;
    std::unique_ptr<std::thread> _backingThread;

    // Requests work from owner until the owner has none
    // left and then returns. process just waits until
    // notified and invokes this.

    void drain() {
      std::shared_ptr<TaskNode<WorkerThread>> oneWork;
      _state = ThreadState::Processing;
      oneWork = _owner->requestWork();
      while(oneWork) {
        oneWork->run();
        oneWork = _owner->requestWork();
      }      
    }
    
  public:
    
    WorkerThread(std::shared_ptr<OwnerType> owner,
                 std::shared_ptr<std::mutex> mtx,
                 std::shared_ptr<std::condition_variable> cv) :
      _shutdown(false),
      _owner(owner),
      _mtx(mtx),
      _cv(cv) {
      _state = ThreadState::Starting;
      _backingThread = std::make_unique<std::thread>([&](){ process(); });
    }

    ~WorkerThread() {
      if (_state != ThreadState::Draining || _state != ThreadState::Shutdown) {
        shutdown();
      }
      if (joinable()) {
        join();
      }
    }

    void process() {
      _state = ThreadState::Ready;

      while(!_shutdown) {
        drain();
        _state = ThreadState::Ready;
        std::unique_lock lock(waitMutex);
        // Wait called like this can have spurious wakeups, but
        // that should be OK. If it seems like it's a problem,
        // we can add a test in a lambda in the call to go
        // back to sleep if we don't want to wake up. It'd
        // probably be about the same amount of processing
        // either way, though.
        _cv->wait(lock, [&](){ return _shutdown || _owner->hasWork();});
      }

      // Call drain again after shutdown to ensure that all work is
      // processed before the thread exits.
      drain();
    }

    bool joinable() {
      return _backingThread->joinable();
    }

    void join() {
      _backingThread->join();
    }

    void shutdown() {
      _shutdown = true;
    }

    ThreadState status() {
      return _state;
    }
    
  };

}
