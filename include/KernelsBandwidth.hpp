#ifndef KERNELSBANDWIDTH_H
#define KERNELSBANDWIDTH_H

#include <random>
#include <hwloc.h>
#include "kernel.hpp"

extern hwloc_topology_t topo;

template<typename T>
T *alloc_init_array(T *array, int n_elems, bool random)
{
  hwloc_nodeset_t aff = hwloc_bitmap_alloc();
  hwloc_bitmap_fill(aff);
  if (array)
    hwloc_free(topo, array, n_elems*sizeof(T));

  array = (T*)hwloc_alloc_membind(topo, n_elems*sizeof(T), aff, HWLOC_MEMBIND_FIRSTTOUCH, 0);
  hwloc_bitmap_free(aff);

  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution(0.0, 1.0);

  auto roll = std::bind(distribution, generator);

  if (random) {
    for (int i = 0; i < n_elems; i++)
      array[i] = (T)roll();
  } else {
    for (int i = 0; i < n_elems; i++)
      array[i] = i;
  }
  return array;
}

void init_array(const std::vector<Param *> *VP);
void copy_array(const std::vector<Param *> *VP);
void readidi(const std::vector<Param *> *VP);
void flush_cache();

#endif
