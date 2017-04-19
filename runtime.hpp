#ifndef RUNTIME_H
#define RUNTIME_H

#include <vector>
#include <deque>
#include <map>
#include <set>
#include <list>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <functional>
#include <cstdint>
#include <condition_variable>
#include "kernel.hpp"

class Executable final {
  intptr_t FunctionPtr;
  typedef void (*FunctionType)(intptr_t);
  FunctionType Callback;
  Executable() = delete;
  Executable &operator=(const Executable &) = delete;

  template <typename Callable>
  static void CallbackFn(intptr_t FunctionPtr) {
    return (*reinterpret_cast<Callable *>(FunctionPtr))();
  }

public:
  template <typename Callable>
  Executable(
      Callable &&Function,
      typename std::enable_if<
          !std::is_same<typename std::remove_reference<Callable>::type,
                        Executable>::value>::type * = nullptr)
      : FunctionPtr(reinterpret_cast<intptr_t>(&Function)),
        Callback(CallbackFn<typename std::remove_reference<Callable>::type>) {}
  void operator()() const;
};

class Task {
  public:
    const Executable e;
    const bool sync;
    const bool flush;
    const std::string name;

    Task(Executable &e, bool sync, bool flush, std::string &&name) : e(e), sync(sync), flush(flush), name(name) {std::cout<<"a\n";};
    Task(Executable &&e, bool sync, bool flush, std::string &&name) : e(e), sync(sync), flush(flush), name(name) {std::cout<<"b\n";};
};

class Watcher {
  public:
    virtual ~Watcher() {};
    virtual void before() = 0;
    virtual void after(const std::string &name) = 0;
    virtual std::string summarize() const = 0;
};

class CycleWatcher : public Watcher {
  std::map<std::string, uint64_t> watchMap_;
  uint64_t start_ = 0;
  uint64_t cyclesBefore_ = 0;
  public:
    virtual void before();
    virtual void after(const std::string &name);
    virtual std::string summarize() const;
};

class TimeWatcher : public Watcher {
  using TimeClock = std::chrono::high_resolution_clock;
  using TimePoint = std::chrono::time_point<TimeClock>;
  using TimeDuration = std::chrono::duration<double>;
  std::map<std::string, TimeDuration> watchMap_;
  uint64_t start_ = 0;
  TimePoint timeBefore_;
  public:
    virtual void before();
    virtual void after(const std::string &name);
    virtual std::string summarize() const;
};

class SyncWatcher : public Watcher {
  std::map<std::string, uint64_t> watchMap_;
  uint64_t begin = 0;
  public:
    virtual void before();
    virtual void after(const std::string &name);
    virtual std::string summarize() const;
};


// CERE popov kernel extraction and replay

struct Thread {
  std::thread t;
  std::deque<Task> q;
  std::mutex m;
  std::atomic_int go = ATOMIC_VAR_INIT(1);
  std::list<Watcher *> watchers_;
};

class Runtime {
  std::map<int, Thread> threads;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::atomic_int there[2] = { ATOMIC_VAR_INIT(0), ATOMIC_VAR_INIT(0) };
  std::atomic_int go[2] = { ATOMIC_VAR_INIT(0), ATOMIC_VAR_INIT(0) };
  std::atomic_int current = ATOMIC_VAR_INIT(0);
  const int max;
  void initThreads(const std::set<int> &physIds);
  public:

  void work(int threadId);

  Runtime(const std::set<int> &physIds);

  template<typename W>
  void addWatcher()
  {
    for (auto &entry : threads) {
      W *w = new W;
      entry.second.watchers_.push_back(w);
    }
  }


  void run(int thread, Task &&t);

  void done();

  static void watcherSummary(int id, const Thread &t);

};

#endif
