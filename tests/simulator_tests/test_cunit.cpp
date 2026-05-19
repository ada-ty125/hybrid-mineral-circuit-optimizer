#include <cassert>
#include <cmath>
#include <iostream>

#include "CUnit.h"

// Simple helper to check if two doubles are effectively equal
bool is_close(double a, double b, double epsilon = 1e-6) { return std::abs(a - b) < epsilon; }

void test_type_a_mass_conservation() {
    CUnit unit;
    unit.n_outputs = 2;  // Type A

    // Set up standard Type A constants manually for testing
    unit.k_matrix[0][0] = 0.008;   // P
    unit.k_matrix[0][1] = 0.006;   // G
    unit.k_matrix[0][2] = 0.0005;  // W

    // Provide a normal incoming feed
    unit.feed[0] = 10.0;
    unit.feed[1] = 15.0;
    unit.feed[2] = 75.0;

    unit.calculate_outputs();

    // Test 1: Mass Conservation Rule (Total Input == Total Output)
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        double input = unit.feed[comp];
        double output = unit.concentrate[0][comp] + unit.tails[comp];
        assert(is_close(input, output) && "Type A Mass balance broken!");
    }
    std::cout << "[PASS] Type A Mass Conservation" << std::endl;
}

int main() {
    std::cout << "CUnit Conservation Tests" << std::endl;

    test_type_a_mass_conservation();
    return 0;
}