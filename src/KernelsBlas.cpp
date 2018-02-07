#include "KernelsBlas.hpp"
#include <lapacke.h>
extern "C" {
#include <cblas.h>
#ifdef USE_ATLAS
  #include <clapack.h>
#endif
}
#include <hwloc.h>
#include <random>
#include <iostream>

using namespace std;

extern hwloc_topology_t topo;

void make_symmetric_positive_definite(const std::vector<Param *> *VP)
{
  ParamImpl<double *> *blockParam = getNthParam<0, double *>(VP);
  ParamImpl<int> *tileSizeParam = getNthParam<0, int>(VP);
  assert(blockParam && tileSizeParam && "One of the expected params to init_dpotrf_block is null!");
  double *a = blockParam->get();
  int blocksize = tileSizeParam->get();

  // A symetric diagonally dominant matrix is symmetric positive definite
  // So we bump the diagonal
  int bump = 128;
  for (int i = 0; i < blocksize; i++) {
    for (int j = 0; j < blocksize; j++) {
      if (i==j)
        a[i*blocksize+j] += bump;
      else if (i < j)
        a[i*blocksize+j] = a[j*blocksize+i];
    }
  }
}

void init_dpotrf_block(const std::vector<Param *> *VP)
{
  // FIXME: dirty copy paste but will do for now
  ParamImpl<double *> *blockParam = getNthParam<0, double *>(VP);
  ParamImpl<int> *tileSizeParam = getNthParam<0, int>(VP);
  assert(blockParam && tileSizeParam && "One of the expected params to init_dpotrf_block is null!");

  double *block = blockParam->get();
  int tileSize = tileSizeParam->get();
  int nElem = tileSize*tileSize;
  hwloc_nodeset_t aff = hwloc_bitmap_alloc();
  hwloc_bitmap_fill(aff);

  if (block)
    hwloc_free(topo, block, nElem*8);

  block = (double*)hwloc_alloc_membind(topo, nElem*8, aff, HWLOC_MEMBIND_FIRSTTOUCH, 0);
  hwloc_bitmap_free(aff);
  default_random_engine generator;
  uniform_real_distribution<double> distribution(-1.0, 1.0);

  auto roll = bind(distribution, generator);

  for (int i = 0; i < tileSize; i++) {
    for (int j = 0; j < tileSize; j++) {
      block[i*tileSize+j] = roll();
    }
  }
  blockParam->set(block);
  make_symmetric_positive_definite(VP);

}

void init_blas_bloc(const std::vector<Param *> *VP)
{
  ParamImpl<double *> *blockParam = getNthParam<0, double *>(VP);
  ParamImpl<int> *tileSizeParam = getNthParam<0, int>(VP);
  assert(blockParam && tileSizeParam && "One of the expected params to init_blas_bloc is null!");

  double *block = blockParam->get();
  int tileSize = tileSizeParam->get();
  int nElem = tileSize*tileSize;
  hwloc_nodeset_t aff = hwloc_bitmap_alloc();
  hwloc_bitmap_fill(aff);

  if (block)
    hwloc_free(topo, block, nElem*8);

  block = (double*)hwloc_alloc_membind(topo, nElem*8, aff, HWLOC_MEMBIND_FIRSTTOUCH, 0);
  hwloc_bitmap_free(aff);
  default_random_engine generator;
  uniform_real_distribution<double> distribution(0.0, 1.0);

  auto roll = bind(distribution, generator);

  for (int i = 0; i < nElem; i++)
    block[i] = roll();
  blockParam->set(block);
}


void kernel_dgemm(const std::vector<Param *> *VP)
{
  ParamImpl<double *> *aParam = getNthParam<0, double *>(VP);
  ParamImpl<double *> *bParam = getNthParam<1, double *>(VP);
  ParamImpl<double *> *cParam = getNthParam<2, double *>(VP);
  ParamImpl<int> *tileSizeParam = getNthParam<0, int>(VP);
  assert(aParam && bParam && cParam && tileSizeParam && "One of the expected params to DGEMM is null!");
  double *a = aParam->get(), *b = bParam->get(), *c = cParam->get();
  int tileSize = tileSizeParam->get();
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, tileSize, tileSize, tileSize, 1,
              a, tileSize, b, tileSize, 1, c, tileSize);
  ;
}

void kernel_dtrsm(const std::vector<Param *> *VP)
{
  ParamImpl<double *> *aParam = getNthParam<0, double *>(VP);
  ParamImpl<double *> *bParam = getNthParam<1, double *>(VP);
  ParamImpl<int> *tileSizeParam = getNthParam<0, int>(VP);
  assert(aParam && bParam && tileSizeParam && "One of the expected params to DTRSM is null!");
  double *a = aParam->get(), *b = bParam->get();
  int tileSize = tileSizeParam->get();
  cblas_dtrsm(CblasRowMajor, CblasLeft, CblasUpper, CblasNoTrans, CblasNonUnit, tileSize, tileSize, 1,
              a, tileSize, b, tileSize);
}

void kernel_dsyrk(const std::vector<Param *> *VP)
{
  ParamImpl<double *> *aParam = getNthParam<0, double *>(VP);
  ParamImpl<double *> *cParam = getNthParam<1, double *>(VP);
  ParamImpl<int> *tileSizeParam = getNthParam<0, int>(VP);
  assert(aParam && cParam && tileSizeParam && "One of the expected params to DSYRK is null!");
  double *a = aParam->get(), *c = cParam->get();
  int tileSize = tileSizeParam->get();
  cblas_dsyrk(CblasRowMajor, CblasUpper, CblasNoTrans, tileSize, tileSize, 1,
              a, tileSize, 1, c, tileSize);
}

void kernel_dpotrf(const std::vector<Param *> *VP)
{
  ParamImpl<double *> *aParam = getNthParam<0, double *>(VP);
  ParamImpl<int> *tileSizeParam = getNthParam<0, int>(VP);
  assert(aParam  && tileSizeParam && "One of the expected params to DPOTRF is null!");
  double *a = aParam->get();
  int tileSize = tileSizeParam->get();

#ifdef USE_ATLAS
  int ret = clapack_dpotrf(CblasRowMajor, CblasUpper, tileSize, a, tileSize);
#else
  int ret = LAPACKE_dpotrf_work(LAPACK_COL_MAJOR, 'U', tileSize, a, tileSize);
#endif
  if (ret != 0) {
    for (int i = 0; i < tileSize; i++) {
      for (int j = 0; j < tileSize; j++) {
        cerr << a[i*tileSize+j] << ", ";
      }
      cerr << "\n";
    }
    cerr << "ret: " << ret << "\n";
    exit(EXIT_FAILURE);
  }
}
