#ifndef KERNEL_H
#define KERNEL_H

#include <tuple>

template <typename... ArgsTypes>
class Kernel {

  protected:

    std::tuple<ArgsTypes...> args;

  public:
    template<typename... ParamsTypes>
      void init(ParamsTypes... params);

    virtual void execute() = 0;

};


#endif
