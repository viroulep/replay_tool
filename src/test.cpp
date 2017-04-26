#include <set>
#include <map>


#include "kblas.hpp"
#include "runtime.hpp"

#include "yaml-cpp/yaml.h"

#include "Support/Casting.h"

using namespace std;

class Test {
public:
  enum TestKind {
    TK_a,
    TK_b
  };
private:
  const TestKind Kind;
public:
  TestKind getKind() const { return Kind; }
  Test(TestKind K) : Kind(K) {}
};

class TestA : public Test {
  int Toto;
public:
  TestA(int T) : Test(TK_a), Toto(T) {}

  static bool classof(const Test *T) {
    return T->getKind() == TK_a;
  }
};

class TestB : public Test {
  double Tata;
public:
  TestB(double T) : Test(TK_b), Tata(T) {}

  static bool classof(const Test *T) {
    return T->getKind() == TK_b;
  }
};


template<typename K>
std::function<void()> make_init(K &k, ParamInt &V)
{
  return [&] { k.init(vector<Param *>({&V})); };
}

template<typename K, typename... ParamsTypes>
std::function<void()> make_exec(K &k)
{
  return [&k] { k.execute(vector<Param *>()); };
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

hwloc_topology_t topo;

using Node=YAML::Node;

void exec_on_and_sync(Runtime &r, Task exec, Task phony, int core, int max)
{
  for (int i = 0; i < max; i += 1) {
    if (i == core)
      r.run(i, std::move(exec));
    else
      r.run(i, std::move(phony));
  }
}

void test_class(const Test &T)
{
  if (isa<TestA>(T))
    cout << "This is a test A\n";
  else if (isa<TestB>(T))
    cout << "This is a test B\n";
  else
    cout << "This is an unknown test\n";
}

int main(int argc, char **argv)
{
  hwloc_topology_init(&topo);
  hwloc_topology_load(topo);
  map<string, Kernel *> kernelInstancesMap;
  map<string, int> paramsInstanceMap;
  Test test1 = TestA(3);
  Test test2 = TestB(4.5);

  ParamImpl<Kernel *> AParam(nullptr);
  vector<Param *> v({ &AParam });
  cout << "Kernel: " << getNthParam<0, Kernel *>(v) << "\n";
  cout << "int: " << getNthParam<0, int>(v) << "\n";

  test_class(test1);
  test_class(test2);

  if (argc > 1) {
    Node config = YAML::LoadFile(argv[1]);
    auto kernels = config["scenarii"]["kernels"].as<map<string, Node>>();
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
  }


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

  Executable phony = [] {
    ;
  };

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

  Task initTask = Task(initA1, true, false, "test");
  Task phonytask = Task(phony, true, false, "phony");

  ParamInt defaultSize(4096);

  //Task t(make_init(a1, nullptr), true, false, "Tinit");
  //Task t_2(make_init(a1, nullptr), true, false, "Tinit");
  //Task t_3(make_init(a1, nullptr), true, false, "Tinit");
  //Task t_4(make_init(a1, nullptr), true, false, "Tinit");
  Task t2(make_init(d2, defaultSize), false, false, "T2init");
  Task t3(make_init(d3, defaultSize), false, false, "T3init");
  //Task tExec(make_exec(d1), true, false, "Texec");
  Task t2Exec(make_exec(d2), true, false, "T2exec");
  Task t3Exec(make_exec(d3), true, false, "T3exec");
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
}
