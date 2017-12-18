#include "kernel.hpp"
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
  return StringSwitch<ParamKind>(name)
    .Case("int", PK_int)
    .Case("double", PK_double)
    .Case("double*", PK_pdouble)
    .Case("float", PK_float)
    .Case("bool", PK_bool)
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
    case PK_pdouble:
      return new ParamImpl<double *>(nullptr);
    case PK_bool:
      return new ParamImpl<bool>(value.as<bool>());
    case PK_unknown:
      return nullptr;
  }
}

string Param::toString() const {
  if (isa<ParamImpl<int>>(this)) {
    return cast<ParamImpl<int>>(this)->toString();
  } else if (isa<ParamImpl<double>>(this)) {
    return cast<ParamImpl<double>>(this)->toString();
  } else if (isa<ParamImpl<double*>>(this)) {
    return cast<ParamImpl<double*>>(this)->toString();
  } else if (isa<ParamImpl<float>>(this)) {
    return cast<ParamImpl<float>>(this)->toString();
  } else if (isa<ParamImpl<bool>>(this)) {
    return cast<ParamImpl<bool>>(this)->toString();
  } else {
    return "unknown";
  }
}
