#include "KernelsCheck.hpp"
#include <numaif.h>
#include <hwloc.h>
#include <lapacke.h>
#include <iostream>
#include <sstream>

using namespace std;

extern hwloc_topology_t topo;

static unsigned int kaapi_numa_getpage_id(const void* addr)
{
  int mode = -1;
  const int err = get_mempolicy(&mode, (unsigned long*)0, 0, (void* )addr, MPOL_F_NODE | MPOL_F_ADDR);
  if (err)
    return (unsigned int)-1;
  /* convert to internal kaapi identifier */
  return mode;
}

void check_affinity(const vector<Param *> *)
{
  int size = 512*512*8;
  hwloc_nodeset_t aff = hwloc_bitmap_alloc();
  hwloc_bitmap_fill(aff);
  double *array = (double*)hwloc_alloc_membind(topo, size, aff, HWLOC_MEMBIND_FIRSTTOUCH, 0);
  //if (posix_memalign((void**)&array, getpagesize(), size))
    //perror("Error allocating memory\n");
  hwloc_bitmap_free(aff);
  int seed[] = {0,0,0,1};
  LAPACKE_dlarnv(1, seed, size/8, array);
  stringstream ret;
  ret << "CPU: " << sched_getcpu() << ", base: " << (void*)array << ", ";
  for (int i = 0; i < 8; i++) {
    ret << "[" << i << "," << kaapi_numa_getpage_id(&array[i*512]) << "],";
  }
  ret << "\n";
  cout << ret.str();
  hwloc_free(topo, array, size);
}
