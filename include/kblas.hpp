#ifndef KBLAS_H
#define KBLAS_H

#include "kernel.hpp"
#include "custom_alloc.hpp"
#include <lapacke.h>
#include <cblas.h>

using ParamInt = ParamImpl<int>;

class AffinityChecker : public Kernel {
  public:
  virtual void init(const std::vector<Param *> &V);
  virtual void execute(const std::vector<Param *> &) {};
  virtual std::string name() { return "AffinityChecker"; }
};

template<typename T>
static void allocateAndInit(T **A, size_t size)
{
  custom_allocate(A, size*size);
  int seed[] = {0,0,0,1};
  LAPACKE_dlarnv(1, seed, size*size, *A);
}


// Blas kernel are special as they operate on the same data type
template <typename... ArgsTypes>
class BlasKernel : public Kernel {
  using BaseElemType = typename std::remove_pointer<typename std::tuple_element<0, std::tuple<ArgsTypes...>>::type>::type;

protected:

  std::tuple<ArgsTypes...> Args;
  std::array<std::size_t, sizeof...(ArgsTypes)> ArgsSizes;

public:

  ~BlasKernel()
  {
    for_each(Args, free);
  }

  virtual void init(const std::vector<Param *> &V) = 0;

  void init()
  {
    for_each(Args, ArgsSizes, allocateAndInit<BaseElemType>);
  }

  virtual std::string name() { return "BlasKernel"; };

};

class DGEMM : public BlasKernel<double *, double *, double *> {
  public:

    void init(const std::vector<Param *> &V);

    void show();

    void execute(const std::vector<Param *> &);

    std::string name() { return "DGEMM"; };
};
#endif
