#include <iostream>
#include <cassert>
#include <cmath>
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

void test_type_b_competitive_math() {
    CUnit unit;
    unit.n_outputs = 3;  // Type B

    // Load competitive constants
    unit.k_matrix[0][0] = 0.007;
    unit.k_matrix[0][1] = 0.001;
    unit.k_matrix[0][2] = 0.001;  // Stream 1
    unit.k_matrix[1][0] = 0.001;
    unit.k_matrix[1][1] = 0.006;
    unit.k_matrix[1][2] = 0.001;  // Stream 2

    unit.feed[0] = 8.0;
    unit.feed[1] = 12.0;
    unit.feed[2] = 80.0;

    unit.calculate_outputs();

    // Test 2: Mass Conservation across 3 streams
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        double input = unit.feed[comp];
        double output = unit.concentrate[0][comp] + unit.concentrate[1][comp] + unit.tails[comp];
        assert(is_close(input, output) && "Type B Mass balance broken!");
    }
    std::cout << "[PASS] Type B Competitive Mass Conservation" << std::endl;
}

void test_zero_flow_safeguard() {
    CUnit unit;
    unit.n_outputs = 2;

    // A running cell first
    unit.feed[0] = 10.0;
    unit.feed[1] = 10.0;
    unit.feed[2] = 10.0;
    unit.calculate_outputs();

    // Make the tank dry out (zero feed)
    unit.feed[0] = 0.0;
    unit.feed[1] = 0.0;
    unit.feed[2] = 0.0;
    unit.calculate_outputs();

    // Verify outputs drop to exactly 0 instead of retaining old states
    assert(is_close(unit.concentrate[0][0], 0.0) &&
           "Safeguard failed: Stale Palusznium left in Conc");
    assert(is_close(unit.tails[2], 0.0) && "Safeguard failed: Stale Waste left in Tails");

    std::cout << "[PASS] Empty Feed Zero-Flow Safeguard" << std::endl;
}

int main() {
    std::cout << "CUnit Conservation Tests" << std::endl;
    test_type_a_mass_conservation();
    test_type_b_competitive_math();
    test_zero_flow_safeguard();
    return 0;
}