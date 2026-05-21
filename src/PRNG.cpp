/**
 * @file PRNG.cpp
 * @brief Implements the default linear congruential random number generator.
 *
 * This file provides the legacy `RandomNumberGenerator` defaults and the
 * `DefaultRandomNumberGenerator` methods used by older GA entry points.
 */

#include <chrono>

#include "RequiredFunctions.h"
#include "PRNG.h"

void seed_rngs(unsigned seed) { DefaultRandomNumberGenerator::global_seed = seed; }

unsigned long DefaultRandomNumberGenerator::global_seed = 0;

int RandomNumberGenerator::RNG_DEFAULT_MIN = 0;
int RandomNumberGenerator::RNG_MAX = 32767;

DefaultRandomNumberGenerator::DefaultRandomNumberGenerator() {
    if (global_seed == 0) {
        next_int = static_cast<unsigned long>(std::time(nullptr));
    } else {
        next_int = global_seed;
    }
}

DefaultRandomNumberGenerator::DefaultRandomNumberGenerator(unsigned seed) { next_int = seed; }

DefaultRandomNumberGenerator::DefaultRandomNumberGenerator(std::seed_seq& seq) {
    std::vector<unsigned> seeds(1);
    seq.generate(seeds.begin(), seeds.end());
    next_int = seeds[0];
}

int DefaultRandomNumberGenerator::lcg_rand() {
    // LCG parameters from POSIX.1-2001
    next_int = (1103515245 * next_int + 12345) % (RNG_MAX + 1);
    return next_int;
}

int DefaultRandomNumberGenerator::rand(int min, int max) {
    return min + lcg_rand() % (max - min + 1);
}

double DefaultRandomNumberGenerator::random(double min, double max) {
    return min + (max - min) * (static_cast<double>(lcg_rand()) / (RNG_MAX - RNG_DEFAULT_MIN));
}
