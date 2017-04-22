#ifndef KERNEL_H
#define KERNEL_H

#include <tuple>


//TODO some wrapper over a generic param
//with custom llvm rtti

class KernelParams {
  public:
  virtual ~KernelParams() {};
};

class Kernel {
  public:
    // until I find something better...
    virtual void init(KernelParams *p) = 0;

    virtual void execute() = 0;

    virtual std::string name() { return "Kernel"; };

    virtual KernelParams *buildParams() = 0;
};


#endif
