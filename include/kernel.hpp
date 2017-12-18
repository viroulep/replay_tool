#ifndef KERNEL_H
#define KERNEL_H

#include <string>
#include <sstream>
#include <vector>
#include <functional>
#include "Support/Casting.h"
#include "yaml-cpp/yaml.h"



enum ParamKind {
#define KERNEL_PARAM_KIND(Name)\
  PK_##Name,
#define KERNEL_PARAM_KIND_PTR(Name)\
  PK_p##Name,
#include "KernelParams.def"
  PK_unknown
};

ParamKind getParamKind(const std::string &name);

class Param {
protected:
  const ParamKind Kind;
public:
  Param(ParamKind K) : Kind(K) {}
  ParamKind getKind() const { return Kind; }
  virtual ~Param() {};
  std::string toString() const;

  static Param *createParam(const std::string &name, const YAML::Node &value);
};

using KernelFunction = std::function<void(const std::vector<Param *> *)>;


template<typename T>
class ParamImpl : public Param {
  T Value;
public:
  ParamImpl(T V);

  T get() const { return Value; }
  T get() { return Value; }
  void set(T Val) { Value = Val; }

  std::string toString() const
  {
    std::stringstream ss;
    ss << Value;
    return ss.str();
  }

  static bool classof(const Param *P);
};

template<std::size_t N, typename T>
ParamImpl<T> *getNthParam(const std::vector<Param *> *V)
{
  std::size_t index = 0;
  ParamImpl<T> *ret;
  for (Param *P : *V) {
    if ((ret = dyn_cast_or_null<ParamImpl<T>>(P))) {
      if (index++ == N)
        return ret;
    }
  }
  return nullptr;
}

#endif
