#ifndef KBLAS_H
#define KBLAS_H

#include "kernel.hpp"
#include "custom_alloc.hpp"

class AffinityChecker : public Kernel {
  public:
  virtual void init(KernelParams *p);
  virtual void execute() {};
  virtual std::string name() { return "AffinityChecker"; }
  virtual KernelParams *buildParams() { return nullptr; }
};

class BlasParams : public KernelParams {
  public:
  ~BlasParams() {};
  int sizeA, sizeB, sizeC;
};

// Blas kernel are special as they operate on the same data type
template <typename... ArgsTypes>
class BlasKernel : public Kernel {
  using BaseElemType = typename std::remove_pointer<typename std::tuple_element<0, std::tuple<ArgsTypes...>>::type>::type;

  protected:

  std::tuple<ArgsTypes...> args;

  public:

  ~BlasKernel()
  {
    for_each(args, free);
  }

  virtual void init(KernelParams *p) = 0;

  template<typename... ParamsTypes>
    void init(ParamsTypes... params)
    {
      std::tuple<ParamsTypes...> someParams = std::make_tuple(params...);
      for_each(args, someParams, custom_allocate<BaseElemType>);
    }

    virtual std::string name() { return "BlasKernel"; };

    virtual KernelParams *buildParams() {
      return new BlasParams;
    }

};

class DGEMM : public BlasKernel<double *, double *, double *> {
  size_t totalSize;
  size_t leadingDimension;
  public:

    void init(KernelParams *p);

    void init(int size);

    void show();

    void execute();

    std::string name() { return "DGEMM"; };
};
#endif
