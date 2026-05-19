#include <cmath>
#include <iostream>
#include <span>

#include "RequiredFunctions.h"

int main() {
    // One type A unit. Concentrate goes to the palusznium product, tailings
    // goes to the gormanium product. The tailings must therefore be priced as
    // gormanium concentrate material, including its waste penalty.
    const int circuit[] = {
        1, 1, 3,  // one input, one unit, three final products
        2,        // unit 0 is type A
        0,        // feed goes to unit 0
        1, 2      // concentrate -> pal product, tailings -> gor product
    };

    const double expected = -38876.11262985238;
    const double got = circuit_performance(std::span<const int>(circuit));

    std::cout << "tailings-to-product routing expected " << expected << ", got " << got << "\n";

    if (std::fabs(got - expected) > 1e-6) {
        return 1;
    }
    return 0;
}
