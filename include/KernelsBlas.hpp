#ifndef KERNELBLAS_H
#define KERNELBLAS_H

#include "kernel.hpp"


void init_dpotrf_block(const std::vector<Param *> *VP);
void make_symmetric_positive_definite(const std::vector<Param *> *VP);
void init_blas_bloc(const std::vector<Param *> *VP);
void kernel_dgemm(const std::vector<Param *> *VP);
void kernel_dtrsm(const std::vector<Param *> *VP);
void kernel_dsyrk(const std::vector<Param *> *VP);
void kernel_dpotrf(const std::vector<Param *> *VP);

#endif
