#include "KernelsBandwidth.hpp"
#include <hwloc.h>
#include <random>
#include <iostream>

using namespace std;

extern hwloc_topology_t topo;

void flush_cache()
{
  // Just create and init a 10*8 MB array, should trash the whole L3 cache
  int size = 20000000;
  double *dummy = alloc_init_array((double *)nullptr, size, false);
  hwloc_free(topo, dummy, size*sizeof(double));
}


void init_array(const vector<Param *> *VP)
{
  ParamImpl<double *> *arrayParam = getNthParam<0, double *>(VP);
  ParamImpl<int> *sizeParam = getNthParam<0, int>(VP);
  ParamImpl<bool> *randomParam = getNthParam<0, bool>(VP);
  assert(arrayParam && sizeParam && "One of the expected params to init_array is null!");

  bool init_random = true;
  if (randomParam)
    init_random = randomParam->get();

  double *array = arrayParam->get();
  int size = sizeParam->get();
  array = alloc_init_array(array, size, init_random);
  arrayParam->set(array);
}

void copy_array(const vector<Param *> *VP)
{
  ParamImpl<double *> *aParam = getNthParam<0, double *>(VP);
  ParamImpl<double *> *bParam = getNthParam<1, double *>(VP);
  ParamImpl<int> *sizeParam = getNthParam<0, int>(VP);
  assert(aParam && bParam && sizeParam && "One of the expected params to init_array is null!");
  volatile double *a = aParam->get();
  volatile double *b = bParam->get();
  int size = sizeParam->get();
  for (int i = 0; i < size; i++) {
    b[i] = a[i];
  }
}
