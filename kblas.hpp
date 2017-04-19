#ifndef KBLAS_H
#define KBLAS_H

#include "kernel.hpp"
#include "custom_alloc.hpp"


// Blas kernel are special as they operate on the same data type
template <typename... ArgsTypes>
class BlasKernel : public Kernel<ArgsTypes...> {
  using BaseElemType = typename std::remove_pointer<typename std::tuple_element<0, std::tuple<ArgsTypes...>>::type>::type;

  public:

  ~BlasKernel()
  {
    for_each(Kernel<ArgsTypes...>::args, free);
  }

  template<typename... ParamsTypes>
    void init(ParamsTypes... params)
    {
      std::tuple<ParamsTypes...> toto = std::make_tuple(params...);
      for_each(Kernel<ArgsTypes...>::args, toto, custom_allocate<BaseElemType>);
    }

};

class DGEMM : BlasKernel<double *, double *, double *> {
  size_t totalSize;
  size_t leadingDimension;
  public:

    void init(int size);

    void show();

    void execute();
};
#endif
