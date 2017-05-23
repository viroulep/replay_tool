#include "runtime.hpp"

#include <chrono>
#include <iostream>
#include <sstream>

#include <pthread.h>

using namespace std;

map<string, KernelFunction> Runtime::kernels_;
set<string> Runtime::watchedKernels_;

uint64_t inline getCycles() {
  uint64_t low, high;
  asm volatile("rdtsc" : "=a" (low), "=d" (high));
  return low | (high << 32);
}

void Executable::operator()() const {
  Callback(FunctionPtr);
}


void CycleWatcher::before()
{
  if (start_) {
    cyclesBefore_ = getCycles() - start_;
  } else {
    start_ = getCycles();
  }
}

void CycleWatcher::after(const string &name)
{
  watchMap_[name].push_back(getCycles() - cyclesBefore_);
}

void TimeWatcher::before()
{
  timeBefore_ = TimeClock::now();
}

void TimeWatcher::after(const string &name)
{
  watchMap_[name].push_back(TimeClock::now() - timeBefore_);
}

void SyncWatcher::before()
{
  begin = getCycles();
}

void SyncWatcher::after(const string &name)
{
  watchMap_[name].push_back(begin);
}

string CycleWatcher::summarize() const
{
  stringstream ss;
  for (auto &entry : watchMap_) {
    if (Runtime::watchedKernels_.count(entry.first) != 1)
      continue;
    ss << "  cycle-" << entry.first << "-" << threadId << ": [";
    for (auto &instance : entry.second)
      ss << instance << ", ";
    ss << "]\n";
  }
  return ss.str();
}

string TimeWatcher::summarize() const
{
  stringstream ss;
  for (auto &entry : watchMap_) {
    if (Runtime::watchedKernels_.count(entry.first) != 1)
      continue;
    for (auto &instance : entry.second)
      ss << name << " " << entry.first << " time-ms " << threadId << " " << instance.count() << "\n";
  }
  return ss.str();
}

string DGEMMFlopsWatcher::summarize() const
{
  stringstream ss;
  for (auto &entry : watchMap_) {
    if (Runtime::watchedKernels_.count(entry.first) != 1)
      continue;
    for (auto &instance : entry.second)
      ss << name << " " << entry.first << " gflops " << threadId << " " << (1e-9*FlopsDgemm)/instance.count() << "\n";
  }
  return ss.str();
}

string SyncWatcher::summarize() const
{
  stringstream ss;
  for (auto &entry : watchMap_) {
    if (Runtime::watchedKernels_.count(entry.first) != 1)
      continue;
    ss << name << " " << "  sync-cycle-" << entry.first << "-" << threadId << ": [";
    for (auto &instance : entry.second)
      ss << instance << ", ";
    ss << "]\n";
  }
  return ss.str();
}

void Runtime::work(int threadId) {
  //FIXME some check on boundaries
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(threadId, &cpuset);
  int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
    cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
  }

  Thread &thread = threads[threadId];
  auto &myQueue = thread.q;
  while (thread.go || !myQueue.empty()) {
    if (!myQueue.empty()) {
      unique_lock<mutex> guard(thread.m);
      Task &task = myQueue.front();
      myQueue.pop_front();
      guard.unlock();
      if (task.sync) {
        // This gives roughly <200 cycles between the start of two synchronous tasks
        int localCurrent = current.load(std::memory_order_acquire);
        int position = there[localCurrent]++;
        if (position + 1 == max) {
          current.store(1 - current);
          there[current] = 0;
          go[current] = 0;
          go[localCurrent].store(1, std::memory_order_release);
        } else {
          while (!go[localCurrent].load(std::memory_order_acquire))
            ;
        }
#if 0
        // This solution through condition variable gives bad synchronous start time:
        // roughly 100000 cycles between starting tasks.
        //
        // 'there' is an atomic, ++ goes through fetch_add
        int position = there++;
        if (position + 1 == max) {
          there = 0;
          cond_.notify_all();
        } else {
          unique_lock<mutex> lock(mutex_);
          cond_.wait(lock);
        }
#endif
      }
      for (int i = 0; i < task.repeat; i++) {
        for (Watcher *w : thread.watchers_)
          w->before();
        //TODO flush
        if (kernels_.find(task.kernelName) != kernels_.end())
          kernels_[task.kernelName](task.kernelParams);
        else {
          cout << "Can't find the kernel " << task.kernelName << ", fatal error\n";
          exit(EXIT_FAILURE);
        }
        for (Watcher *w : thread.watchers_)
          w->after(task.name);
      }
    }
    //this_thread::sleep_for(chrono::seconds(1));
  }
}

void Runtime::run(int thread, Task &code)
{
  run(thread, move(code));
}

void Runtime::run(int thread, Task &&code)
{
  if (threads.count(thread) != 1) {
    cerr << "Trying to run on a non declared thread (" << thread << "), aborting\n";
    exit(EXIT_FAILURE);
  }
  Thread &t = threads[thread];
  lock_guard<mutex> guard(t.m);
  t.q.push_back(std::forward<Task>(code));
}

void Runtime::watcherSummary(int id, const Thread &t)
{
  for (Watcher *w : t.watchers_)
    cout << w->summarize();
}

void Runtime::done()
{
  // Two separate loop to avoid thread spinning on 'go' while joining!
  for (auto &entry : threads)
    entry.second.go = 0;
  for (auto &entry : threads) {
    std::thread &t = entry.second.t;
    t.join();
  }
  for (auto &entry : threads) {
    Runtime::watcherSummary(entry.first, entry.second);
    for (Watcher *w : entry.second.watchers_)
      delete w;
  }
}

void Runtime::initThreads(const set<int> &physIds)
{
  for (int cpuId : physIds)
    threads[cpuId].t = thread([this, cpuId] {work(cpuId);});
}

Runtime::Runtime(const set<int> &physIds) : max(physIds.size())
{
  initThreads(physIds);
}

