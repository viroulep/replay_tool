
#include "kblas.hpp"

#include <iostream>
#include <lapacke.h>
#include <cblas.h>

using namespace std;

void DGEMM::init(int size)
{
  totalSize = size*size;
  leadingDimension = size;
  BlasKernel::init(totalSize, totalSize, totalSize);
  //TODO random init
  double *a, *b, *c;
  tie(a, b, c) = args;
  int seed[] = {0,0,0,1};
  LAPACKE_dlarnv(1, seed, totalSize, a);
  LAPACKE_dlarnv(1, seed, totalSize, b);
  LAPACKE_dlarnv(1, seed, totalSize, c);
}

void DGEMM::show()
{
  double *a, *b, *c;
  tie(a, b, c) = args;
  cout << "start\n";
  for (unsigned long i = 0; i < totalSize; i++) {
    cout << c[i] << ", ";
    if ((i+1) % leadingDimension == 0)
      cout << "\n";
  }
  cout << "\nend\n";
}

void DGEMM::execute() {
  double *a, *b, *c;
  tie(a, b, c) = args;
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, leadingDimension, leadingDimension, leadingDimension, 1,
              a, leadingDimension, b, leadingDimension, 1, c, leadingDimension);
}
