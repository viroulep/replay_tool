#include "kernel.hpp"
#include "kblas.hpp"
#include "Support/StringSwitch.h"

using namespace std;

// Create constructors declaration

#define KERNEL_PARAM_KIND(Name)\
  template<>\
  ParamImpl<Name>::ParamImpl(Name V) : Param(PK_##Name), Value(V) {}
#define KERNEL_PARAM_KIND_PTR(Name)\
  template<>\
  ParamImpl<Name *>::ParamImpl(Name *V) : Param(PK_p##Name), Value(V) {}
#include "KernelParams.def"

// Create classof-s

#define KERNEL_PARAM_KIND(Name)\
  template<>\
  bool ParamImpl<Name>::classof(const Param *P) { return P->getKind() == PK_##Name; }
#define KERNEL_PARAM_KIND_PTR(Name)\
  template<>\
  bool ParamImpl<Name *>::classof(const Param *P) { return P->getKind() == PK_p##Name; }
#include "KernelParams.def"


KernelKind getKernelKind(const string &name)
{
  return StringSwitch<KernelKind>(name)
    .Case("AffinityChecker", KK_AffinityChecker)
    .Case("DGEMM", KK_DGEMM)
    .Default(KK_unknown);
}

Kernel *Kernel::createKernel(const string &name)
{
  switch (getKernelKind(name)) {
    case KK_AffinityChecker:
      return new AffinityChecker;
    case KK_DGEMM:
      return new DGEMM;
    case KK_unknown:
      return nullptr;
  }
}

std::string Kernel::name()
{
  switch (Kind) {
    case KK_AffinityChecker:
      return "AffinityChecker";
    case KK_DGEMM:
      return "DGEMM";
    case KK_unknown:
      return "unknown";
  }
}

