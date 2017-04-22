
#include "kblas.hpp"
#include "custom_alloc.hpp"

#include <iostream>
#include <sstream>
#include <lapacke.h>
#include <cblas.h>
#include <numaif.h>

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

void AffinityChecker::init(KernelParams *p)
{
  int size = 4096*8;
  double *array;
  if (posix_memalign((void**)&array, getpagesize(), size))
    perror("Error allocating memory\n");
  int seed[] = {0,0,0,1};
  LAPACKE_dlarnv(1, seed, size/8, array);
  stringstream ret;
  ret << "CPU: " << sched_getcpu() << ", base: " << (void*)array << ", ";
  for (int i = 0; i < 8; i++) {
    ret << "[" << i << "," << kaapi_numa_getpage_id(&array[i*512]) << "],";
  }
  ret << "\n";
  cout << ret.str();
  free(array);
}

void DGEMM::init(KernelParams *p) {
  BlasParams *casted = dynamic_cast<BlasParams *>(p);
  init(casted->sizeA);
}

void DGEMM::init(int size)
{
  totalSize = size*size;
  leadingDimension = size;
  BlasKernel::init(totalSize, totalSize, totalSize);
  double *a, *b, *c;
  tie(a, b, c) = args;
  int seed[] = {0,0,0,1};
  LAPACKE_dlarnv(1, seed, totalSize, a);
  LAPACKE_dlarnv(1, seed, totalSize, b);
  LAPACKE_dlarnv(1, seed, totalSize, c);
}

void DGEMM::show()
{
  double *a, *b, *c;
  tie(a, b, c) = args;
  cout << "start\n";
  for (unsigned long i = 0; i < totalSize; i++) {
    cout << c[i] << ", ";
    if ((i+1) % leadingDimension == 0)
      cout << "\n";
  }
  cout << "\nend\n";
}

void DGEMM::execute() {
  double *a, *b, *c;
  tie(a, b, c) = args;
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, leadingDimension, leadingDimension, leadingDimension, 1,
              a, leadingDimension, b, leadingDimension, 1, c, leadingDimension);
}
