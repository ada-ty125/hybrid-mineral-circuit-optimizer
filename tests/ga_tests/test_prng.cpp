#include "PRNG.h"
#include <iostream>

int main() {
    DefaultRandomNumberGenerator rng(12345);  // Seed with a fixed value for reproducibility

    // Test integer random generation
    for (int i = 0; i < 10; ++i) {
        int rand_int = rng.rand(1, 100);
        std::cout << "Random Integer: " << rand_int << std::endl;
        if (rand_int < 1 || rand_int > 100) {
            std::cerr << "Error: Random integer out of bounds!" << std::endl;
            return 1;
        }
    }

    // Maybe do some stats tests here.

    // Test double random generation
    for (int i = 0; i < 10; ++i) {
        double rand_double = rng.random(0.0, 1.0);
        std::cout << "Random Double: " << rand_double << std::endl;
        if (rand_double < 0.0 || rand_double > 1.0) {
            std::cerr << "Error: Random double out of bounds!" << std::endl;
            return 1;
        }
    }

    return 0;
}
