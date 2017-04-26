#ifndef KBLAS_H
#define KBLAS_H

#include "kernel.hpp"
#include "custom_alloc.hpp"
#include <lapacke.h>
#include <cblas.h>

using ParamInt = ParamImpl<int>;

class AffinityChecker : public Kernel {
public:
  AffinityChecker() : Kernel(KK_AffinityChecker) {}
  virtual void init(const std::vector<Param *> &V);
  virtual void execute(const std::vector<Param *> &) {};
  static bool classof(const Kernel *K) { return K->getKind() == KK_AffinityChecker; }
};

template<typename T>
static void allocateAndInit(T **A, size_t size)
{
  custom_allocate(A, size*size);
  int seed[] = {0,0,0,1};
  LAPACKE_dlarnv(1, seed, size*size, *A);
}


// Blas are special as they operate on the same data type
template <typename... ArgsTypes>
class BlasArgs {
  using BaseElemType = typename std::remove_pointer<typename std::tuple_element<0, std::tuple<ArgsTypes...>>::type>::type;

protected:

  std::tuple<ArgsTypes...> Args;
  std::array<std::size_t, sizeof...(ArgsTypes)> ArgsSizes;

public:

  ~BlasArgs()
  {
    for_each(Args, free);
  }

  void init()
  {
    for_each(Args, ArgsSizes, allocateAndInit<BaseElemType>);
  }
};

class DGEMM : public BlasArgs<double *, double *, double *>, public Kernel {
public:

  DGEMM() : BlasArgs(), Kernel(KK_DGEMM) {}

  void init(const std::vector<Param *> &V);

  void show();

  void execute(const std::vector<Param *> &);

  static bool classof(const Kernel *K) { return K->getKind() == KK_DGEMM; }
};
#endif
