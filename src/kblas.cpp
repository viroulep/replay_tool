
#include "kblas.hpp"
#include "custom_alloc.hpp"

#include <iostream>
#include <sstream>
#include <numaif.h>
#include <hwloc.h>

using namespace std;

static unsigned int kaapi_numa_getpage_id(const void* addr)
{
  int mode = -1;
  const int err = get_mempolicy(&mode, (unsigned long*)0, 0, (void* )addr, MPOL_F_NODE | MPOL_F_ADDR);
  if (err)
    return (unsigned int)-1;
  /* convert to internal kaapi identifier */
  return mode;
}

void AffinityChecker::init(const vector<Param *> &)
{
  int size = 4096*8;
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

void DGEMM::init(const vector<Param *> &V)
{
  ParamInt *sizeParam = getNthParam<0, int>(V);
  // We *need* at least one size
  assert(sizeParam);
  ArgsSizes[0] = sizeParam->get();
  ArgsSizes[1] = ((sizeParam = getNthParam<1, int>(V))) ? sizeParam->get() : ArgsSizes.front();
  ArgsSizes[2] = ((sizeParam = getNthParam<2, int>(V))) ? sizeParam->get() : ArgsSizes.front();

  BlasKernel::init();
}

void DGEMM::show()
{
  double *a, *b, *c;
  tie(a, b, c) = Args;
  cout << "start\n";
  for (unsigned long i = 0; i < ArgsSizes[2]*ArgsSizes[2]; i++) {
    cout << c[i] << ", ";
    if ((i+1) % ArgsSizes[2] == 0)
      cout << "\n";
  }
  cout << "\nend\n";
}

void DGEMM::execute(const vector<Param *> &) {
  double *a, *b, *c;
  tie(a, b, c) = Args;
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, ArgsSizes[0], ArgsSizes[1], ArgsSizes[2], 1,
              a, ArgsSizes[0], b, ArgsSizes[1], 1, c, ArgsSizes[2]);
}
