#include "kernel.hpp"

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

