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

ParamKind getParamKind(const string &name)
{
  if (getKernelKind(name) != KK_unknown)
    return PK_pKernel;
  return StringSwitch<ParamKind>(name)
    .Case("int", PK_int)
    .Case("double", PK_double)
    .Case("float", PK_float)
    .Default(PK_unknown);
}

Param *Param::createParam(const string &name, const YAML::Node &value)
{
  switch (getParamKind(name)) {
    case PK_int:
      return new ParamImpl<int>(value.as<int>());
    case PK_double:
      return new ParamImpl<double>(value.as<double>());
    case PK_float:
      return new ParamImpl<float>(value.as<float>());
    case PK_pKernel:
      return new ParamImpl<Kernel *>(Kernel::createKernel(name));
    case PK_unknown:
      return nullptr;
  }
}

string Param::toString() const {
  if (isa<ParamImpl<int>>(this)) {
    return cast<ParamImpl<int>>(this)->toString();
  } else if (isa<ParamImpl<double>>(this)) {
    return cast<ParamImpl<double>>(this)->toString();
  } else if (isa<ParamImpl<float>>(this)) {
    return cast<ParamImpl<float>>(this)->toString();
  } else if (isa<ParamImpl<Kernel *>>(this)) {
    return cast<ParamImpl<Kernel *>>(this)->toString();
  } else {
    return "unknown";
  }
}

std::ostream& operator<<(std::ostream& os, const Kernel *obj)
{
  os << obj->name();
  return os;
}

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

std::string Kernel::name() const
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

