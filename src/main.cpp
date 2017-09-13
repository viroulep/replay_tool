#include <set>
#include <map>
#include <memory>
#include <hwloc.h>


#include "KernelsBlas.hpp"
#include "KernelsCheck.hpp"
#include "runtime.hpp"

#include "yaml-cpp/yaml.h"

#include "Support/Casting.h"

using namespace std;



hwloc_topology_t topo;

using Node=YAML::Node;
using NodeType=YAML::NodeType;

int main(int argc, char **argv)
{
  hwloc_topology_init(&topo);
  hwloc_topology_load(topo);
  map<string, int> paramsInstanceMap;
  map<string, Param *> dataMap;
  // FIXME: annoying to manage...
  set<vector<Param *> *> paramsAllocated;
  Runtime::kernels_.insert(make_pair("init_blas_bloc", init_blas_bloc));
  Runtime::kernels_.insert(make_pair("dgemm", kernel_dgemm));
  Runtime::kernels_.insert(make_pair("dtrsm", kernel_dtrsm));
  Runtime::kernels_.insert(make_pair("dsyrk", kernel_dsyrk));
  Runtime::kernels_.insert(make_pair("check_affinity", check_affinity));
  Runtime::watchedKernels_.insert("dgemm");
  Runtime::watchedKernels_.insert("dtrsm");
  Runtime::watchedKernels_.insert("dsyrk");
  // FIXME: obviously to specific...
  int blockSize = 512;

  vector<Node> actions;
  string name = "unknown";

  if (argc > 1) {
    // Get variables
    // Get parameters
    // Get actions
    // Get watchers
    Node config = YAML::LoadFile(argv[1]);
    auto data = config["scenarii"]["data"].as<map<string, Node>>();
    for (auto &var : data) {
      dataMap[var.first] = Param::createParam(var.second["type"].as<string>(), var.second["value"]);
    }
    //for (auto &var : dataMap) {
      //cout << var.first << ": " << var.second->toString() << "\n";
    //}
    actions = config["scenarii"]["actions"].as<vector<Node>>();
    if (config["scenarii"]["name"].IsDefined())
      name = config["scenarii"]["name"].as<string>();

    /*
    auto kernels = config["scenarii"]["actions"].as<map<string, Node>>();
    cout << "Detecting kernels:\n";
    for (auto &entry : kernels) {
      cout << "Entry is: " << entry.first;
      Kernel *k = Kernel::createKernel(entry.second["type"].as<string>());
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
    */
  }

  //cout << "Actions: \n";

  set<int> cores;
  // TODO: repeat the following for all "params" in the scenario!
  for (auto &a : actions) {
    if (a.IsMap()) {
      cores.insert(a["core"].as<int>());
    }
  }

  if (dataMap.count("bs") > 0)
    // yepyepyep, could be a segfault if the user is not aware
    blockSize = dyn_cast_or_null<ParamImpl<int>>(dataMap["bs"])->get();

  Runtime runtime(cores);
  //runtime.addWatcher<CycleWatcher>();
  runtime.addWatcher<TimeWatcher>(name);
  runtime.addWatcher<DGEMMFlopsWatcher>(name, blockSize);
  runtime.addWatcher<DTRSMFlopsWatcher>(name, blockSize);
  runtime.addWatcher<DSYRKFlopsWatcher>(name, blockSize);
  //runtime.addWatcher<SyncWatcher>();
  // TODO: perfcounter watcher

  for (auto &a : actions) {
    switch (a.Type()) {
      case NodeType::Scalar:
        if (a.as<string>() == "sync") {
          for (int i : cores) {
            runtime.run(i, Task("dummy", nullptr, true, false, 1, "dummy"));
          }
        }
        break;
      case NodeType::Map:
        {
          auto actionInfo = a.as<map<string, Node>>();
          int core = actionInfo["core"].as<int>();
          string kernelName = actionInfo["kernel"].as<string>();
          bool sync = false;
          int repeat = 1;
          if (actionInfo["sync"].IsDefined())
            sync = actionInfo["sync"].as<bool>();
          if (actionInfo["repeat"].IsDefined())
            repeat = actionInfo["repeat"].as<int>();
          vector<Param *> *params = new vector<Param *>;
          paramsAllocated.insert(params);
          stringstream debugString;
          debugString << "-- Task creation: " << kernelName;
          if (actionInfo["params"].IsSequence()) {
            for (string &paramName : actionInfo["params"].as<vector<string>>()) {
              debugString << ", " << paramName;
              params->push_back(dataMap[paramName]);
            }
          }
          debugString << ", on core " << core;
          debugString << ", repeat: " << repeat;
          debugString << ", sync: " << sync << "\n";
          if (0)
            cout << debugString.str();

          if (Runtime::kernels_.find(kernelName) != Runtime::kernels_.end()) {
            // FIXME: That's ugly :(
            runtime.run(core, Task(kernelName, params, sync, false, repeat, kernelName));
            ;
          } else {
            cout << "Can't find kernel " << kernelName << "\n";
            exit(EXIT_FAILURE);
          }
          break;
        }
      case NodeType::Sequence:
      case NodeType::Undefined:
      case NodeType::Null:
        cerr << "Error: Unrecognized action in the scenario file\n";
        return 1;
        break;
    }
  }

  runtime.done();
  for (auto *p : paramsAllocated)
    delete p;
  for (auto &entry : dataMap)
    delete entry.second;

  hwloc_topology_destroy(topo);
  return 0;
#if 0

  set<int> threads;
  //threads.insert(0);
  //threads.insert(1);
  //threads.insert(2);
  //threads.insert(3);
  //threads.insert(32);
  //threads.insert(4);
  Runtime r(threads);
  r.addWatcher<CycleWatcher>();
  r.addWatcher<TimeWatcher>();
  r.addWatcher<SyncWatcher>();

  DGEMM d1, d2, d3;
  AffinityChecker *a1 = cast<AffinityChecker>(Kernel::createKernel("AffinityChecker"));

  //Executable phony = [] {
    //;
  //};

  Executable execCode1 = [&] {
    d1.execute(vector<Param *>());
  };

  Executable initA1 = [a1] {
    a1->init(vector<Param *>());
  };

  Executable &&toto = [&] {
    cout << "toto\n";
  };

  initA1();

  Task initTask = Task(initA1, true, false, 1, "test");
  Task phonytask = Task(phony, true, false, 1, "phony");

  ParamInt defaultSize(4096);

  //Task t(make_init(a1, nullptr), true, false, "Tinit");
  //Task t_2(make_init(a1, nullptr), true, false, "Tinit");
  //Task t_3(make_init(a1, nullptr), true, false, "Tinit");
  //Task t_4(make_init(a1, nullptr), true, false, "Tinit");
  Task t2(make_init(d2, defaultSize), false, false, 1, "T2init");
  Task t3(make_init(d3, defaultSize), false, false, 1, "T3init");
  //Task tExec(make_exec(d1), true, false, "Texec");
  Task t2Exec(make_exec(d2), true, false, 1, "T2exec");
  Task t3Exec(make_exec(d3), true, false, 1, "T3exec");
  //Task tExec2(execCode, true, false, "Texecbis");
  //Task t2Exec2(make_exec(d2), true, false, "T2execbis");

  //r.run(0, t2);
  //r.run(0, t2Exec);
  //r.run(1, t3);
  //r.run(1, t3Exec);
  //r.run(0, initTask);
  //r.run(1, phonytask);
  //for (int i = 0; i < 4; i++) {
    //if (i == 0)
      //r.run(i, Task([]{ cout << "coucou\n"; }, false, false, "task"));
  //}
  //for (int i = 0; i < 1; i++) {
    //exec_on_and_sync(r, initTask, phonytask, 0, 4);
    //exec_on_and_sync(r, initTask, phonytask, 1, 4);
    //exec_on_and_sync(r, initTask, phonytask, 2, 4);
    //exec_on_and_sync(r, initTask, phonytask, 3, 4);
    //exec_on_and_sync(r, initTask, phonytask, 32, 33);
  //}
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

  delete a1;

  hwloc_topology_destroy(topo);
  return 0;
#endif
}
