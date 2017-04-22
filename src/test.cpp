#include <set>
#include <map>


#include "kblas.hpp"
#include "runtime.hpp"

#include "yaml-cpp/yaml.h"

using namespace std;

template<typename K, typename... ParamsTypes>
std::function<void()> make_init(K &k, ParamsTypes ...params)
{
  return [&k, params...] { k.init(params...); };
}

template<typename K, typename... ParamsTypes>
std::function<void()> make_exec(K &k)
{
  return [&k] { k.execute(); };
}

template<typename K, typename... ParamsTypes>
int run(Runtime &r, K &k, int toto, ParamsTypes ...params)
{
  Executable init = make_init(k, params...);
  Executable exec = make_exec(k);
  r.run(toto, make_init(k, params...));
  r.run(toto, make_exec(k));
  //this_thread::sleep_for(chrono::seconds(1));
}

//Un visitor qui prend name->type
//

Kernel *getKernel(const string &s)
{
  Kernel *ret = nullptr;
  // FIXME: import kernels at compile time
  // FIXME: -fno-rtti and custom rtti
  if (s == "DGEMM") {
    ret = dynamic_cast<Kernel *>(new DGEMM);
  }
  return ret;
}

using Node=YAML::Node;

void exec_on_and_sync(Runtime &r, Task exec, Task phony, int core, int max)
{
  for (int i = 0; i < max; i++) {
    if (i == core)
      r.run(i, std::move(exec));
    else
      r.run(i, std::move(phony));
  }
}

int main(int argc, char **argv)
{

  map<string, Kernel *> kernelInstancesMap;
  map<string, int> paramsInstanceMap;

  if (argc > 1) {
    Node config = YAML::LoadFile(argv[1]);
    auto kernels = config["scenarii"]["kernels"].as<map<string, Node>>();
    cout << "Detecting kernels:\n";
    for (auto &entry : kernels) {
      cout << "Entry is: " << entry.first;
      Kernel *k = getKernel(entry.second["type"].as<string>());
      cout << " Instanciated kernel is: ";
      if (k)
        cout << k->name() << "\n";
      else
        cout << "null...\n";
    }
    cout << "Detecting execution_params:\n";
    Node executionNode = config["scenarii"]["execution"];
    auto params = executionNode["execution_params"].as<map<string, Node>>();
    for (auto &entry : params) {
      cout << "Found: " << entry.first << "=" << entry.second[0].as<int>() << "\n";
      paramsInstanceMap[entry.first] = entry.second[0].as<int>();
    }
  }

  DGEMM a, b;

  set<int> threads;
  threads.insert(0);
  threads.insert(1);
  threads.insert(2);
  threads.insert(3);
  //threads.insert(4);
  Runtime r(threads);
  r.addWatcher<CycleWatcher>();
  r.addWatcher<TimeWatcher>();
  r.addWatcher<SyncWatcher>();

  DGEMM d1, d2, d3;
  AffinityChecker a1;

  Executable phony = [] {
    ;
  };

  Executable execCode1 = [&] {
    d1.execute();
  };

  Executable initA1 = [&] {
    a1.init(nullptr);
  };

  Executable &&toto = [&] {
    cout << "toto\n";
  };

  Task initTask = Task(initA1, true, false, "test");
  Task phonytask = Task(phony, true, false, "phony");


  //Task t(make_init(a1, nullptr), true, false, "Tinit");
  //Task t_2(make_init(a1, nullptr), true, false, "Tinit");
  //Task t_3(make_init(a1, nullptr), true, false, "Tinit");
  //Task t_4(make_init(a1, nullptr), true, false, "Tinit");
  //Task t2(make_init(d2, 4096), false, false, "T2init");
  //Task t3(make_init(d3, 4096), false, false, "T3init");
  //Task tExec(make_exec(d1), true, false, "Texec");
  //Task t2Exec(make_exec(d2), true, false, "T2exec");
  //Task t3Exec(make_exec(d3), true, false, "T3exec");
  //Task tExec2(execCode, true, false, "Texecbis");
  //Task t2Exec2(make_exec(d2), true, false, "T2execbis");

  //for (int i = 0; i < 4; i++) {
    //if (i == 0)
      //r.run(i, Task([]{ cout << "coucou\n"; }, false, false, "task"));
  //}
  for (int i = 0; i < 100; i++) {
    exec_on_and_sync(r, initTask, phonytask, 0, 4);
    exec_on_and_sync(r, initTask, phonytask, 1, 4);
    exec_on_and_sync(r, initTask, phonytask, 2, 4);
    exec_on_and_sync(r, initTask, phonytask, 3, 4);
  }
  //this_thread::sleep_for(chrono::seconds(1));

  //r.run(1, std::move(t));
  //r.run(2, std::move(t));
  //r.run(3, std::move(t));
  //r.run(0, std::move(tExec));
  //r.run(4, std::move(t2));
  //r.run(4, std::move(t2Exec));
  //r.run(4, std::move(t3));
  //r.run(4, std::move(t3Exec));
  //r.run(1, std::move(t3));
  //r.run(1, std::move(t4));
  //r.run(2, make_init(d2, 256));
  //r.run(1, execCode);
  //r.run(2, make_exec(d2));


  //run(r, d2, 2, 512);


  r.done();

  return 0;
}
