#ifndef KERNEL_H
#define KERNEL_H

#include <string>
#include <vector>
#include <functional>
#include "Support/Casting.h"

//TODO some wrapper over a generic param
//with custom llvm rtti

class Param;

enum KernelKind {
  KK_AffinityChecker,
  KK_DGEMM,
  KK_unknown
};

KernelKind getKernelKind(const std::string &name);

class Kernel {
  const KernelKind Kind;

  public:
    Kernel(KernelKind K) : Kind(K) {}
    KernelKind getKind() const { return Kind; }
    virtual ~Kernel() {}

    virtual void init(const std::vector<Param *> &V) = 0;

    virtual void execute(const std::vector<Param *> &V) = 0;

    std::string name();

    static Kernel *createKernel(const std::string &name);
};

enum ParamKind {
#define KERNEL_PARAM_KIND(Name)\
  PK_##Name,
#define KERNEL_PARAM_KIND_PTR(Name)\
  PK_p##Name,
#include "KernelParams.def"
  PK_unknown
};

class Param {
  const ParamKind Kind;
public:
  Param(ParamKind K) : Kind(K) {}
  ParamKind getKind() const { return Kind; }
  virtual ~Param() {};
};

template<typename T>
class ParamImpl : public Param {
  T Value;
public:
  ParamImpl(T V);

  T get() { return Value; }

  static bool classof(const Param *P);
};

template<std::size_t N, typename T>
ParamImpl<T> *getNthParam(const std::vector<Param *> &V)
{
  std::size_t index = 0;
  ParamImpl<T> *ret;
  for (Param *P : V) {
    if ((ret = dyn_cast<ParamImpl<T>>(P))) {
      if (index++ == N)
        return ret;
    }
  }
  return nullptr;
}



#endif
