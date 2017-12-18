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
  double *dummy = alloc_init_array(nullptr, size, false);
  hwloc_free(topo, dummy, size*sizeof(double));
}

double *alloc_init_array(double *array, int size, bool random)
{
  cout << "start_init (random=" << random << ")\n";
  hwloc_nodeset_t aff = hwloc_bitmap_alloc();
  hwloc_bitmap_fill(aff);
  if (array)
    hwloc_free(topo, array, size*sizeof(double));

  array = (double*)hwloc_alloc_membind(topo, size*sizeof(double), aff, HWLOC_MEMBIND_FIRSTTOUCH, 0);
  hwloc_bitmap_free(aff);

  default_random_engine generator;
  uniform_real_distribution<double> distribution(0.0, 1.0);

  auto roll = bind(distribution, generator);

  if (random) {
    for (int i = 0; i < size; i++)
      array[i] = roll();
  } else {
    for (int i = 0; i < size; i++)
      array[i] = i;
  }
  return array;
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
