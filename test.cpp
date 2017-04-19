#include <set>

#include "kblas.hpp"
#include "runtime.hpp"

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



int main()
{

  DGEMM a, b;

  set<int> threads;
  threads.insert(0);
  threads.insert(4);
  //threads.insert(4);
  Runtime r(threads);
  r.addWatcher<CycleWatcher>();
  r.addWatcher<TimeWatcher>();
  r.addWatcher<SyncWatcher>();

  DGEMM d1, d2, d3;

  Executable execCode1 = [&] {
    d1.execute();
  };

  Task t(make_init(d1, 4096), false, false, "Tinit");
  Task t2(make_init(d2, 4096), false, false, "T2init");
  Task t3(make_init(d3, 4096), false, false, "T3init");
  Task tExec(make_exec(d1), true, false, "Texec");
  Task t2Exec(make_exec(d2), true, false, "T2exec");
  Task t3Exec(make_exec(d3), true, false, "T3exec");
  //Task tExec2(execCode, true, false, "Texecbis");
  //Task t2Exec2(make_exec(d2), true, false, "T2execbis");

  r.run(0, std::move(t));
  r.run(0, std::move(tExec));
  r.run(4, std::move(t2));
  r.run(4, std::move(t2Exec));
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
