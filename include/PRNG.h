/**
 * @file PRNG.h
 * @brief Declares the default pseudo-random number generator implementation.
 *
 * `DefaultRandomNumberGenerator` implements the project `RandomNumberGenerator`
 * interface using a lightweight linear congruential generator for legacy GA
 * code paths.
 */

#pragma once

#include "RequiredFunctions.h"

// Default random number generator implementation using a linear congruential generator (LCG).

class DefaultRandomNumberGenerator : public RandomNumberGenerator {
    unsigned long next_int;  // State of the LCG
  protected:
    static unsigned long global_seed;  // Global seed for all future instances of the default RNG

    int lcg_rand();

  public:
    DefaultRandomNumberGenerator();  // Seed with current time
    DefaultRandomNumberGenerator(unsigned);
    DefaultRandomNumberGenerator(std::seed_seq& seq);

    int rand(int min, int max);
    double random(double min, double max);

    friend void seed_rngs(unsigned seed);
};
