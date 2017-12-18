#ifndef KERNELSBANDWIDTH_H
#define KERNELSBANDWIDTH_H

#include "kernel.hpp"

double *alloc_init_array(double *array, int size, bool random);
void init_array(const std::vector<Param *> *VP);
void copy_array(const std::vector<Param *> *VP);
void flush_cache();

#endif
