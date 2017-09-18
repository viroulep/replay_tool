#include "KernelsBlas.hpp"
#include <lapacke.h>
#include <cblas.h>
#include <hwloc.h>

extern hwloc_topology_t topo;

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
  int seed[] = {0,0,0,1};
  LAPACKE_dlarnv(1, seed, nElem, block);
  for (int i = 0; i < nElem; i++)
    if (block[i] < 0)
      block[i] = (-block[i]);
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
  LAPACKE_dpotrf_work(LAPACK_COL_MAJOR, 'U', tileSize, a, tileSize);
}
