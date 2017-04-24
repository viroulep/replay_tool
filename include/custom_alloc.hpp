#ifndef CUSTOM_ALLOC_H
#define CUSTOM_ALLOC_H

#include <tuple>
#include <utility>
#include <stdlib.h>
#include <unistd.h>
#include <hwloc.h>

template<std::size_t I = 0, typename FuncT, typename... Tp, typename... Pp>
  inline typename std::enable_if<I == sizeof...(Tp), void>::type
for_each(std::tuple<Tp...> &, std::tuple<Pp...> &, FuncT)
{ }

template<std::size_t I = 0, typename FuncT, typename... Tp, typename... Pp>
  inline typename std::enable_if<I < sizeof...(Tp), void>::type
for_each(std::tuple<Tp...>& t, std::tuple<Pp...> &p, FuncT f)
{
  f(&std::get<I>(t), std::get<I>(p));
  for_each<I + 1, FuncT, Tp...>(t, p, f);
}

template<std::size_t I = 0, typename FuncT, typename... Tp>
  inline typename std::enable_if<I == sizeof...(Tp), void>::type
for_each(std::tuple<Tp...> &, FuncT)
{ }

template<std::size_t I = 0, typename FuncT, typename... Tp>
  inline typename std::enable_if<I < sizeof...(Tp), void>::type
for_each(std::tuple<Tp...>& t, FuncT f)
{
  f(std::get<I>(t));
  for_each<I + 1, FuncT, Tp...>(t, f);
}

template<typename T>
void custom_allocate(T **a, size_t size) {
  T *tmp;
  if (posix_memalign((void**)&tmp, getpagesize(), size * sizeof(T)))
    perror("Error allocating memory\n");
  // Allow re-init
  if (*a)
    free(*a);
  *a = new(tmp) T[size];
}

extern hwloc_topology_t topo;

#endif
