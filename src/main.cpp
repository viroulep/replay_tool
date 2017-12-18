#include <set>
#include <map>
#include <memory>
#include <hwloc.h>


#include "KernelsBandwidth.hpp"
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
  map<string, Param *> dataMap;
  // FIXME: annoying to manage...
  set<vector<Param *> *> paramsAllocated;
  Runtime::kernels_.insert(make_pair("init_blas_bloc", init_blas_bloc));
  Runtime::kernels_.insert(make_pair("dgemm", kernel_dgemm));
  Runtime::kernels_.insert(make_pair("dtrsm", kernel_dtrsm));
  Runtime::kernels_.insert(make_pair("dsyrk", kernel_dsyrk));
  Runtime::kernels_.insert(make_pair("dpotrf", kernel_dpotrf));
  Runtime::kernels_.insert(make_pair("init_symmetric", init_dpotrf_block));
  Runtime::kernels_.insert(make_pair("init_array", init_array));
  Runtime::kernels_.insert(make_pair("copy", copy_array));
  Runtime::kernels_.insert(make_pair("check_affinity", check_affinity));
  Runtime::watchedKernels_.insert("dgemm");
  Runtime::watchedKernels_.insert("copy");
  Runtime::watchedKernels_.insert("dtrsm");
  Runtime::watchedKernels_.insert("dsyrk");
  Runtime::watchedKernels_.insert("dpotrf");

  vector<Node> actions;
  string name = "unknown";

  if (argc != 2) {
    cerr << "Please provide a scenario file!\n";
    exit(EXIT_FAILURE);
  }
  // Get variables
  // Get parameters
  // Get actions
  // Get watchers
  Node config = YAML::LoadFile(argv[1]);
  auto data = config["scenarii"]["data"].as<map<string, Node>>();
  for (auto &var : data) {
    dataMap[var.first] = Param::createParam(var.second["type"].as<string>(), var.second["value"]);
  }

  actions = config["scenarii"]["actions"].as<vector<Node>>();
  if (config["scenarii"]["name"].IsDefined())
    name = config["scenarii"]["name"].as<string>();


  set<int> cores;
  // TODO: repeat the following for all "params" in the scenario!
  for (auto &a : actions) {
    if (a.IsMap()) {
      cores.insert(a["core"].as<int>());
    }
  }


  Runtime runtime(cores, name);


  if (config["scenarii"]["watchers"].IsDefined()) {
    auto watchers = config["scenarii"]["watchers"].as<map<string, Node>>();
    for (auto &entry : watchers) {
      if (entry.first == "papi") {
        vector<string> counters = entry.second.as<vector<string>>();
        runtime.addWatcher<PerfCtrWatcher>(counters);
      } else if (entry.first == "flops_dgemm") {
        auto params = entry.second.as<vector<string>>();
        // FIXME: handle error if not enough params
        int blockSize = dyn_cast_or_null<ParamImpl<int>>(dataMap[params.front()])->get();
        runtime.addWatcher<DGEMMFlopsWatcher>(blockSize);
      } else if (entry.first == "flops_dtrsm") {
        auto params = entry.second.as<vector<string>>();
        int blockSize = dyn_cast_or_null<ParamImpl<int>>(dataMap[params.front()])->get();
        runtime.addWatcher<DTRSMFlopsWatcher>(blockSize);
      } else if (entry.first == "flops_dsyrk") {
        auto params = entry.second.as<vector<string>>();
        int blockSize = dyn_cast_or_null<ParamImpl<int>>(dataMap[params.front()])->get();
        runtime.addWatcher<DSYRKFlopsWatcher>(blockSize);
      } else if (entry.first == "flops_dpotrf") {
        auto params = entry.second.as<vector<string>>();
        int blockSize = dyn_cast_or_null<ParamImpl<int>>(dataMap[params.front()])->get();
        runtime.addWatcher<DPOTRFFlopsWatcher>(blockSize);
      } else if (entry.first == "size") {
        auto params = entry.second.as<vector<string>>();
        int nElements = dyn_cast_or_null<ParamImpl<int>>(dataMap[params.front()])->get();
        runtime.addWatcher<ArraySizeWatcher>(nElements);
      } else if (entry.first == "time") {
        runtime.addWatcher<TimeWatcher>();
      } else if (entry.first == "sync") {
        runtime.addWatcher<SyncWatcher>();
      }
    }
  }


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
          string name = kernelName;
          bool sync = false;
          bool flush = false;
          int repeat = 1;
          if (!actionInfo["name"].IsNull())
            name = actionInfo["name"].as<string>();
          if (!actionInfo["flush"].IsNull())
            flush = actionInfo["flush"].as<bool>();
          if (!actionInfo["sync"].IsNull())
            sync = actionInfo["sync"].as<bool>();
          if (!actionInfo["repeat"].IsNull())
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
          debugString << ", sync: " << sync;
          debugString << ", name: " << name;
          debugString << ", flush: " << flush << "\n";
          if (0)
            cout << debugString.str();

          if (Runtime::kernels_.find(kernelName) != Runtime::kernels_.end()) {
            // FIXME: That's ugly :(
            runtime.run(core, Task(kernelName, params, sync, flush, repeat, name));
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
}
