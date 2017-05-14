#include <set>
#include <map>
#include <memory>


#include "kblas.hpp"
#include "runtime.hpp"

#include "yaml-cpp/yaml.h"

#include "Support/Casting.h"

using namespace std;


/*
template<typename K>
std::function<void()> make_init(K &k, ParamInt &V)
{
  return [&] { k.init(vector<Param *>({&V})); };
}

template<typename K>
std::function<void()> make_init(K &k, vector<Param *> V)
{
  return [=, &k] { k.init(V); };
}

template<typename K, typename... ParamsTypes>
std::function<void()> make_exec(K &k)
{
  return [&k] { k.execute(vector<Param *>()); };
}
*/

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

hwloc_topology_t topo;

using Node=YAML::Node;
using NodeType=YAML::NodeType;

Executable make_action(Kernel *K, vector<Param *> *V, const string &Action)
{
  cout << "Vsize(1): " << V->size() << "\n";
  cout << "V:" << V << "\n";
  vector<Param *> *titi = V;
  if (Action == "init")
    return [K, titi] { cout << "V:" << titi << "\n" << "Vsize: " << titi->size() << "\n"; K->init(titi); };
  else if (Action == "exec")
    return [K] { K->execute(nullptr); };
  else
    return [=] { ; };
}

int main(int argc, char **argv)
{
  hwloc_topology_init(&topo);
  hwloc_topology_load(topo);
  map<string, Kernel *> kernelInstancesMap;
  map<string, int> paramsInstanceMap;
  map<string, Param *> dataMap;
  // FIXME: annoying to manage...
  set<vector<Param *> *> paramsAllocated;

  vector<Node> actions;

  ParamImpl<Kernel *> AParam(nullptr);
  vector<Param *> v({ &AParam });
  cout << "Kernel: " << getNthParam<0, Kernel *>(v) << "\n";
  cout << "int: " << getNthParam<0, int>(v) << "\n";

  if (argc > 1) {
    // Get variables
    // Get parameters
    // Get actions
    Node config = YAML::LoadFile(argv[1]);
    auto data = config["scenarii"]["data"].as<map<string, Node>>();
    for (auto &var : data) {
      dataMap[var.first] = Param::createParam(var.second["type"].as<string>(), var.second["value"]);
    }
    for (auto &var : dataMap) {
      cout << var.first << ": " << var.second->toString() << "\n";
    }
    actions = config["scenarii"]["actions"].as<vector<Node>>();

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

  cout << "Actions: \n";
  Executable phony = [] {
    ;
  };

  set<int> cores;
  // TODO: repeat the following for all "params" in the scenario!
  for (auto &a : actions) {
    if (a.IsMap()) {
      cores.insert(a["core"].as<int>());
    }
  }

  Runtime runtime(cores);
  runtime.addWatcher<CycleWatcher>();
  runtime.addWatcher<TimeWatcher>();
  runtime.addWatcher<SyncWatcher>();
  // TODO: perfcounter watcher

  for (auto &a : actions) {
    switch (a.Type()) {
      case NodeType::Scalar:
        if (a.as<string>() == "sync") {
          for (int i : cores) {
            runtime.run(i, Task(phony, true, false, 1, "sync"));
          }
        }
        break;
      case NodeType::Map:
        {
          auto actionInfo = a.as<map<string, Node>>();
          int core = actionInfo["core"].as<int>();
          string kernelName = actionInfo["kernel"].as<string>();
          string actionName = actionInfo["action"].as<string>();
          bool sync = false;
          if (!actionInfo["sync"].IsNull())
            sync = actionInfo["sync"].as<bool>();
          vector<Param *> *params = new vector<Param *>;
          paramsAllocated.insert(params);

          cout << "-- Task creation: " << kernelName << "." << actionName;
          if (actionInfo["params"].IsSequence()) {
            for (string &paramName : actionInfo["params"].as<vector<string>>()) {
              cout << ", " << paramName;
              params->push_back(dataMap[paramName]);
            }
          }
          cout << ", on core " << core;
          cout << ", sync: " << sync << "\n";

          ParamImpl<Kernel *> *pk = dyn_cast_or_null<ParamImpl<Kernel *>>(dataMap[kernelName]);
          if (pk) {
            Kernel *k = pk->get();
            // FIXME: That's ugly :(
            if (actionName == "init") {
              Executable e = [=] { k->init(params); };
              runtime.run(core, Task(e, sync, false, 1, kernelName+"/"+actionName));
            } else if (actionName == "exec") {
              Executable e = [=] { k->execute(nullptr); };
              runtime.run(core, Task(e, sync, false, 1, kernelName+"/"+actionName));
            }
          } else {
            cout << "Can't find kernel " << kernelName << "\n";
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
