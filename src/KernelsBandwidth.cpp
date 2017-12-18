#include "KernelsBandwidth.hpp"
#include <hwloc.h>
#include <random>

using namespace std;

extern hwloc_topology_t topo;

void init_array(const std::vector<Param *> *VP)
{
  ParamImpl<double *> *arrayParam = getNthParam<0, double *>(VP);
  ParamImpl<int> *sizeParam = getNthParam<0, int>(VP);
  assert(arrayParam && sizeParam && "One of the expected params to init_array is null!");

  double *array = arrayParam->get();
  int size = sizeParam->get();
  hwloc_nodeset_t aff = hwloc_bitmap_alloc();
  hwloc_bitmap_fill(aff);
  if (array)
    hwloc_free(topo, array, size*sizeof(double));

  array = (double*)hwloc_alloc_membind(topo, size*sizeof(double), aff, HWLOC_MEMBIND_FIRSTTOUCH, 0);
  hwloc_bitmap_free(aff);

  default_random_engine generator;
  uniform_real_distribution<double> distribution(0.0, 1.0);

  auto roll = bind(distribution, generator);

  for (int i = 0; i < size; i++)
    array[i] = roll();
  arrayParam->set(array);
}

