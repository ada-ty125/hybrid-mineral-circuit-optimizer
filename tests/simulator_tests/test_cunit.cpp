#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include "CUnit.h"
#include "CCircuit.h" r

bool is_close(double a, double b, double epsilon = 1e-6) { return std::abs(a - b) < epsilon; }

void test_type_a_mass_conservation() {
    CUnit unit;
    unit.n_outputs = 2;  // 1 Concentrate + 1 Tailings
    unit.unit_type = 0;  // Tells unit to look at params.k_TypeA

    Simulator_Parameters test_params;
    test_params.k_TypeA[0][0] = 0.008;   // Palusznium
    test_params.k_TypeA[0][1] = 0.006;   // Gormanium
    test_params.k_TypeA[0][2] = 0.0005;  // Waste

    // Physical sizing defaults
    test_params.tank_volume = 10.0;
    test_params.fluid_density = 3000.0;

    // Provide an incoming mass feed
    unit.feed[0] = 10.0;
    unit.feed[1] = 15.0;
    unit.feed[2] = 75.0;

    unit.calculate_outputs(test_params);

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
    unit.n_outputs = 3;  // 2 Concentrates + 1 Tailings
    unit.unit_type = 1;  // Tells unit to look at params.k_TypeB

    // Create local parameters configuring multi-stream competitive kinetics
    Simulator_Parameters test_params;
    test_params.tank_volume = 10.0;
    test_params.fluid_density = 3000.0;

    // Load competitive constants into Type B matrix
    test_params.k_TypeB[0][0] = 0.007;
    test_params.k_TypeB[0][1] = 0.001;
    test_params.k_TypeB[0][2] = 0.001;  // Stream 0
    test_params.k_TypeB[1][0] = 0.001;
    test_params.k_TypeB[1][1] = 0.006;
    test_params.k_TypeB[1][2] = 0.001;  // Stream 1

    unit.feed[0] = 8.0;
    unit.feed[1] = 12.0;
    unit.feed[2] = 80.0;

    unit.calculate_outputs(test_params);

    // Test 2: Mass Conservation across 3 physical output streams
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
    unit.unit_type = 0;

    Simulator_Parameters test_params;
    test_params.tank_volume = 10.0;
    test_params.fluid_density = 3000.0;
    test_params.min_denominator = 1e-12;

    // First cycle with a normal mass flow baseline
    unit.feed[0] = 10.0;
    unit.feed[1] = 10.0;
    unit.feed[2] = 10.0;
    unit.calculate_outputs(test_params);

    // Complete feed starvation
    unit.feed[0] = 0.0;
    unit.feed[1] = 0.0;
    unit.feed[2] = 0.0;
    unit.calculate_outputs(test_params);

    // Verify system scales outputs straight down to zero cleanly
    assert(is_close(unit.concentrate[0][0], 0.0) &&
           "Safeguard failed: Stale Palusznium left in Conc");
    assert(is_close(unit.tails[2], 0.0) && "Safeguard failed: Stale Waste left in Tails");

    std::cout << "[PASS] Empty Feed Zero-Flow Safeguard" << std::endl;
}

int main() {
    std::cout << "CUnit Dynamic Conservation Tests" << std::endl;
    test_type_a_mass_conservation();
    test_type_b_competitive_math();
    test_zero_flow_safeguard();
    return 0;
}