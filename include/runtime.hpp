#ifndef RUNTIME_H
#define RUNTIME_H

#include <vector>
#include <deque>
#include <map>
#include <array>
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
#include <chrono>
#include <iostream>
#include <sstream>
#include <papi.h>
#include "kernel.hpp"

const int MAX_EVENTS = 4;

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
    const std::string kernelName;
    const std::vector<Param *> *kernelParams;
    const bool sync;
    const bool flush;
    const int repeat;
    const std::string name;

    Task(const std::string &kernelName, const std::vector<Param *> *kernelParams, bool sync, bool flush, int repeat, const std::string &name) : kernelName(kernelName), kernelParams(kernelParams), sync(sync), flush(flush), repeat(repeat), name(name) {};
};

// CERE popov kernel extraction and replay

class AbstractWatcher;

struct Thread {
  std::thread t;
  std::deque<Task> q;
  std::mutex m;
  std::atomic_int go = ATOMIC_VAR_INIT(1);
  std::list<AbstractWatcher *> watchers_;
};

class Runtime {
  std::map<int, Thread> threads;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::atomic_int there[2] = { ATOMIC_VAR_INIT(0), ATOMIC_VAR_INIT(0) };
  std::atomic_int go[2] = { ATOMIC_VAR_INIT(0), ATOMIC_VAR_INIT(0) };
  std::atomic_int current = ATOMIC_VAR_INIT(0);
  const int MAX_THREADS;
  const std::string name_;
  void initThreads(const std::set<int> &physIds);
  public:

  void work(int threadId);

  Runtime(const std::set<int> &physIds, const std::string &name);

  template<typename W, typename... Args>
  void addWatcher(Args... args)
  {
    for (auto &entry : threads) {
      W *w = new W(entry.first, args...);
      entry.second.watchers_.push_back(w);
    }
  }


  void run(int thread, Task &t);
  void run(int thread, Task &&t);

  void done();

  static std::map<std::string, KernelFunction> kernels_;
  static std::set<std::string> watchedKernels_;
};

class AbstractWatcher {
  protected:
    const int threadId;
  public:
    static void headers(std::list<AbstractWatcher *> watchList);
    static void summarize(const std::string &prefix, int totalThread, int tId, std::list<AbstractWatcher *> watchList);
    AbstractWatcher(int threadId) : threadId(threadId) {};
    virtual ~AbstractWatcher() {};
    virtual void before() = 0;
    virtual void after(const std::string &name) = 0;
    virtual std::vector<std::reference_wrapper<const std::string>> keys() = 0;
    virtual size_t size() const = 0;
    virtual std::string dataHeader() const = 0;
    virtual std::string dataEntry(const std::string &name, int iteration) const = 0;
};


template<typename EntryType>
class Watcher : public AbstractWatcher {
  protected:
    std::map<std::string, std::vector<EntryType>> watchMap_;
  public:
    Watcher(int threadId) : AbstractWatcher(threadId) {};
    virtual ~Watcher() {};
    virtual std::vector<std::reference_wrapper<const std::string>> keys()
    {
      std::vector<std::reference_wrapper<const std::string>> ret;
      for (auto &entry : watchMap_) {
        if (Runtime::watchedKernels_.count(entry.first) != 1)
          continue;
        ret.push_back(entry.first);
      }
      return ret;
    }
    virtual size_t size() const { return watchMap_.empty() ? 0 : watchMap_.begin()->second.size(); };
};


using CycleEntry = uint64_t;
class CycleWatcher : public Watcher<CycleEntry> {
  CycleEntry start_ = 0;
  CycleEntry cyclesBefore_ = 0;
  public:
    CycleWatcher(int threadId) : Watcher(threadId) {};
    virtual void before();
    virtual void after(const std::string &name);
    virtual std::string dataHeader() const;
    virtual std::string dataEntry(const std::string &name, int iteration) const;
};

using ArraySizeEntry = int;
class ArraySizeWatcher : public Watcher<ArraySizeEntry> {
  const ArraySizeEntry size_;
  public:
    ArraySizeWatcher(int threadId, ArraySizeEntry size) : Watcher(threadId), size_(size) {};
    virtual void before();
    virtual void after(const std::string &name);
    virtual std::string dataHeader() const;
    virtual std::string dataEntry(const std::string &name, int iteration) const;
};

using TimeClock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<TimeClock>;
using TimeDuration = std::chrono::duration<double>;
class TimeWatcher : public Watcher<TimeDuration> {
  protected:
    TimePoint timeBefore_;
  public:
    TimeWatcher(int threadId) : Watcher(threadId) {};
    virtual ~TimeWatcher() {};
    virtual void before();
    virtual void after(const std::string &name);
    virtual std::string dataHeader() const;
    virtual std::string dataEntry(const std::string &name, int iteration) const;
};

using PerfRecordValue = std::array<long long, MAX_EVENTS>;
class PerfCtrWatcher : public Watcher<PerfRecordValue> {
  public:
    const int numEvents;
    std::array<int, MAX_EVENTS> events;
    std::array<std::string, MAX_EVENTS> eventsNames;
  protected:
    PerfRecordValue valuesBefore_;
    bool started_ = false;
  public:
    PerfCtrWatcher(int threadId, std::vector<std::string> &eventsVector);
    virtual ~PerfCtrWatcher() {};
    virtual void before();
    virtual void after(const std::string &name);
    virtual std::string dataHeader() const;
    virtual std::string dataEntry(const std::string &name, int iteration) const;
};

// See LAWN 41 and test/flops.h in plasma
static double fmuls_gemm(double m, double n, double k)
{ return m*n*k; }

static double fadds_gemm(double m, double n, double k)
{ return m*n*k; }

static double fmuls_syrk(double k, double n)
{ return 0.5 * k * n * (n+1); }

static double fadds_syrk(double k, double n)
{ return 0.5 * k * n * (n+1); }

static double fmuls_trsm(double m, double n)
{ return 0.5 * (n) * (m) * ((m)+1); }

static double fadds_trsm(double m, double n)
{ return 0.5 * (n) * (m) * ((m)-1); }

static double fmuls_potrf(double n)
{ return (1./6.)*n*n*n + 0.5*n*n + (1./3.)*n; }

static double fadds_potrf(double n)
{ return (1./6.)*n*n*n - (1./6.)*n; }
//#define FMULS_POTRF(__n) ((double)(__n) * (((1. / 6.) * (double)(__n) + 0.5) * (double)(__n) + (1. / 3.)))
//#define FADDS_POTRF(__n) ((double)(__n) * (((1. / 6.) * (double)(__n)      ) * (double)(__n) - (1. / 6.)))


class FlopsWatcher : public TimeWatcher {
  public:
    const double FLOPS;
    FlopsWatcher(int threadId, double flops) : TimeWatcher(threadId), FLOPS(flops) {};
    virtual std::string dataHeader() const;
    virtual std::string dataEntry(const std::string &name, int iteration) const;
};

class DGEMMFlopsWatcher : public FlopsWatcher {
  public:
    DGEMMFlopsWatcher(int threadId, int blockSize) : FlopsWatcher(threadId, fmuls_gemm(blockSize, blockSize, blockSize) + fadds_gemm(blockSize, blockSize, blockSize)) {};
};

class DTRSMFlopsWatcher : public FlopsWatcher {
  public:
    DTRSMFlopsWatcher(int threadId, int blockSize) : FlopsWatcher(threadId, fmuls_trsm(blockSize, blockSize) + fadds_trsm(blockSize, blockSize)) {};
};

class DSYRKFlopsWatcher : public FlopsWatcher {
  public:
    DSYRKFlopsWatcher(int threadId, int blockSize) : FlopsWatcher(threadId, fmuls_syrk(blockSize, blockSize) + fadds_syrk(blockSize, blockSize)) {};
};

class DPOTRFFlopsWatcher : public FlopsWatcher {
  public:
    DPOTRFFlopsWatcher(int threadId, int blockSize) : FlopsWatcher(threadId, fmuls_potrf(blockSize) + fadds_potrf(blockSize)) {};
};

class SyncWatcher : public Watcher<CycleEntry> {
  CycleEntry begin = 0;
  public:
    SyncWatcher(int threadId) : Watcher(threadId) {};
    virtual void before();
    virtual void after(const std::string &name);
    virtual std::string dataHeader() const;
    virtual std::string dataEntry(const std::string &name, int iteration) const;
};



#endif
