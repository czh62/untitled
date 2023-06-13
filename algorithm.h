#ifndef ALGORITHM_H
#define ALGORITHM_H
#include <random>

int64_t mod_mux(int64_t x, int64_t y, int64_t n);
int64_t mod_pow(int64_t a, uint64_t b, int64_t c);
bool IsPrime(int64_t n);
int64_t ProduceRandomPrime();
int64_t PrimitiveElement(int64_t n);

#endif // ALGORITHM_H
