#include "runtime.hpp"
#include "KernelsBlas.hpp"

#include <pthread.h>

using namespace std;

map<string, KernelFunction> Runtime::kernels_;
set<string> Runtime::watchedKernels_;

void dummy(const vector<Param *> *)
{}


uint64_t inline getCycles() {
  uint64_t low, high;
  asm volatile("rdtsc" : "=a" (low), "=d" (high));
  return low | (high << 32);
}

void Executable::operator()() const {
  Callback(FunctionPtr);
}

void AbstractWatcher::headers(list<AbstractWatcher *> watcherList)
{
  if (watcherList.empty())
    return;
  cout << "Exp total_cores kernel thread";
  for (auto &watcher : watcherList)
    cout << watcher->dataHeader();
  cout << "\n";
}

void AbstractWatcher::summarize(const string &prefix, int totalThread, int tId, list<AbstractWatcher *> watcherList)
{
  if (watcherList.empty())
    return;
  auto keys = watcherList.front()->keys();
  for (const string &kernelName : keys) {
    for (int i = 0; i < watcherList.front()->size(); i++) {
      cout << prefix << " " << totalThread << " " << kernelName << " " << tId << " ";
      for (auto &watcher : watcherList) {
        cout << watcher->dataEntry(kernelName, i);
      }
      cout << "\n";
    }
  }
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

void PerfCtrWatcher::before()
{
  if (!started_) {
    if (PAPI_start_counters(events.data(), numEvents) != PAPI_OK) {
      cerr << "Before::Can't read PAPI counters, exiting\n";
      exit(EXIT_FAILURE);
    }
    started_ = true;
  } else {
    if (PAPI_read_counters(valuesBefore_.data(), numEvents) != PAPI_OK) {
      cerr << "Before::Can't read PAPI counters, exiting\n";
      exit(EXIT_FAILURE);
    }
  }
}

void PerfCtrWatcher::after(const string &name)
{
  PerfRecordValue now;
  //if (PAPI_stop_counters(now.data(), numEvents) != PAPI_OK) {
    //cerr << "After::Can't read PAPI counters, exiting\n";
    //exit(EXIT_FAILURE);
  //}
  if (PAPI_read_counters(now.data(), numEvents) != PAPI_OK) {
    cerr << "After::Can't read PAPI counters, exiting\n";
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < numEvents; i++)
    now[i] -= valuesBefore_[i];

  watchMap_[name].push_back(now);
}

string CycleWatcher::dataHeader() const
{
  return " cycle";
}

string TimeWatcher::dataHeader() const
{
  return " time-ms";
}

string PerfCtrWatcher::dataHeader() const
{
  stringstream ss;
  for (const string &name : eventsNames)
    ss << " " << name;
  return ss.str();
}

string FlopsWatcher::dataHeader() const
{
  return " gflops";
}

string SyncWatcher::dataHeader() const
{
  return " cycle-sync";
}

string PerfCtrWatcher::dataEntry(const string &name, int iteration) const
{
  stringstream ss;
  auto &entry = watchMap_.at(name)[iteration];
  for (int i = 0; i < numEvents; i++)
    ss << " " << entry[i];
  return ss.str();
}

PerfCtrWatcher::PerfCtrWatcher(int threadId,
                               vector<string> &eventsVector) : Watcher(threadId), numEvents(eventsVector.size())
{
  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    cerr << "Can't init papi\n";
    exit(EXIT_FAILURE);
  }
  //cerr << this_thread::get_id();
  if (PAPI_thread_init((unsigned long (*)())this_thread::get_id) != PAPI_OK) {
    cerr << "Can't init multithread support in papi\n";
    exit(EXIT_FAILURE);
  }

  if (eventsVector.size() > events.size()) {
    cerr << "Too many events!";
    exit(EXIT_FAILURE);
  }
  int i = 0;
  for (string &s : eventsVector) {
    int eventCode;
    if (PAPI_event_name_to_code(const_cast<char *>(s.c_str()), &eventCode) != PAPI_OK) {
      cerr << "Couldn't find counter '" << s << "', exiting\n";
      exit(EXIT_FAILURE);
    }
    events[i] = eventCode;
    eventsNames[i++] = s;
  }
}

string SyncWatcher::dataEntry(const std::string &name, int iteration) const
{
  std::stringstream ss;
  ss << " " << watchMap_.at(name)[iteration];
  return ss.str();
}

string CycleWatcher::dataEntry(const std::string &name, int iteration) const
{
  std::stringstream ss;
  ss << " " << watchMap_.at(name)[iteration];
  return ss.str();
}

string TimeWatcher::dataEntry(const string &name, int iteration) const
{
  stringstream ss;
  ss << " " << chrono::duration_cast<chrono::duration<double, milli>>(watchMap_.at(name)[iteration]).count();
  return ss.str();
}

string FlopsWatcher::dataEntry(const string &name, int iteration) const
{
  stringstream ss;
  ss << " " << (1e-9*FLOPS)/watchMap_.at(name)[iteration].count();
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
    cerr << "With thread id " << threadId << "\n";
    cerr << EFAULT << "\n";
    cerr << EINVAL << "\n";
    cerr << ESRCH << "\n";
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
        if (position + 1 == MAX_THREADS) {
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
        // FIXME: beurk
        if (task.kernelName == "dpotrf") {
          make_symmetric_positive_definite(task.kernelParams);
        }
        for (AbstractWatcher *w : thread.watchers_) {
          if (task.kernelName != "dummy" && task.kernelName != "init_blas_bloc" && task.kernelName != "init_symmetric")
            w->before();
        }
        //TODO flush
        if (kernels_.find(task.kernelName) != kernels_.end()) {
          kernels_[task.kernelName](task.kernelParams);
        } else {
          cout << "Can't find the kernel " << task.kernelName << ", fatal error\n";
          exit(EXIT_FAILURE);
        }
        for (AbstractWatcher *w : thread.watchers_) {
          if (task.kernelName != "sync" && task.kernelName != "init_blas_bloc" && task.kernelName != "init_symmetric")
            w->after(task.name);
        }
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

void Runtime::done()
{
  // Two separate loop to avoid thread spinning on 'go' while joining!
  for (auto &entry : threads)
    entry.second.go = 0;
  for (auto &entry : threads) {
    std::thread &t = entry.second.t;
    t.join();
  }
  AbstractWatcher::headers(threads.begin()->second.watchers_);
  for (auto &entry : threads) {
    AbstractWatcher::summarize(name_, MAX_THREADS, entry.first, entry.second.watchers_);
    for (AbstractWatcher *w : entry.second.watchers_)
      delete w;
  }
}

void Runtime::initThreads(const set<int> &physIds)
{
  Runtime::kernels_.insert(make_pair("dummy", dummy));
  for (int cpuId : physIds) {
    Thread &t = threads[cpuId];
    t.q.push_back(Task("dummy", nullptr, true, false, 1, "start_sync"));
  }
  for (int cpuId : physIds)
    threads[cpuId].t = thread([this, cpuId] {work(cpuId);});
}

Runtime::Runtime(const set<int> &physIds, const string &name) : MAX_THREADS(physIds.size()), name_(name)
{
  initThreads(physIds);
}

