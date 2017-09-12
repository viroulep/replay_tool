#ifndef KERNELBLAS_H
#define KERNELBLAS_H

#include "kernel.hpp"


void init_blas_bloc(const std::vector<Param *> *VP);
void kernel_dgemm(const std::vector<Param *> *VP);
void kernel_dtrsm(const std::vector<Param *> *VP);

#endif
