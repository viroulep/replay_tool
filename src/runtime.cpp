#include "runtime.hpp"
#include "custom_alloc.hpp"

#include <chrono>
#include <iostream>
#include <sstream>

#include <pthread.h>

using namespace std;

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
  watchMap_.insert(make_pair(name, getCycles() - cyclesBefore_));
}

void TimeWatcher::before()
{
  timeBefore_ = TimeClock::now();
}

void TimeWatcher::after(const string &name)
{
  watchMap_.insert(make_pair(name, TimeClock::now() - timeBefore_));
}

void SyncWatcher::before()
{
  begin = getCycles();
}

void SyncWatcher::after(const string &name)
{
  watchMap_.insert(make_pair(name, begin));
}

string CycleWatcher::summarize() const
{
  stringstream ss;
  ss << "CycleWatcher (name: cycles)\n";
  for (auto &entry : watchMap_) {
    ss << "  " << entry.first << ": " << entry.second << "\n";
  }
  return ss.str();
}

string TimeWatcher::summarize() const
{
  stringstream ss;
  ss << "TimeWatcher (name: seconds)\n";
  for (auto &entry : watchMap_) {
    ss << "  " << entry.first << ": " << entry.second.count() << "\n";
  }
  return ss.str();
}

string SyncWatcher::summarize() const
{
  stringstream ss;
  ss << "SyncWacther (name: begin)\n";
  for (auto &entry : watchMap_) {
    ss << "  " << entry.first << ": " << entry.second << "\n";
  }
  return ss.str();
}

void Runtime::work(int threadId) {
  Thread &thread = threads[threadId];
  auto &myQueue = thread.q;
  while (thread.go || !myQueue.empty()) {
    if (!myQueue.empty()) {
      unique_lock<mutex> guard(thread.m);
      Task &task = myQueue.front();
      myQueue.pop_front();
      guard.unlock();
      const Executable &e = task.e;
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
      for (Watcher *w : thread.watchers_)
        w->before();
      //TODO repeat
      //TODO flush
      e();
      for (Watcher *w : thread.watchers_)
        w->after(task.name);
    }
    //this_thread::sleep_for(chrono::seconds(1));
  }
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
  cout << "Watchers summary for thread " << id << "\n";
  for (Watcher *w : t.watchers_) {
    cout << w->summarize();
  }
  cout << "--------------\n";
}

void Runtime::done()
{
  for (auto &entry : threads) {
    std::thread &t = entry.second.t;
    entry.second.go = 0;
    t.join();
    Runtime::watcherSummary(entry.first, entry.second);
    for (Watcher *w : entry.second.watchers_)
      delete w;
  }
}

void Runtime::initThreads(const set<int> &physIds)
{
  for (int cpuId : physIds) {
    threads[cpuId].t = thread([this, cpuId] {work(cpuId);});
    //FIXME some check on boundaries
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuId, &cpuset);
    int rc = pthread_setaffinity_np(threads[cpuId].t.native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
      cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
  }
}

Runtime::Runtime(const set<int> &physIds) : max(physIds.size())
{
  initThreads(physIds);
}
