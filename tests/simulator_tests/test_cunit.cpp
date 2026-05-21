/**
 * @file test_cunit.cpp
 * @brief Unit tests for separator cell mass balance and recovery calculations.
 *
 * The checks cover Type A and Type B mass conservation, recovery values, and
 * zero-feed behavior for individual `CUnit` instances.
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "CUnit.h"
#include "CCircuit.h"

bool is_close(double a, double b, double eps = 1e-6) { return std::abs(a - b) < eps; }

void prepare_unit(CUnit& unit, const Simulator_Parameters& params) {
    const int n_comp = params.n_components;
    const int n_conc = unit.n_outputs - 1;

    unit.feed.assign(n_comp, 0.0);
    unit.old_feed.assign(n_comp, 0.0);
    unit.tails.assign(n_comp, 0.0);
    unit.total_recovery.assign(n_comp, 0.0);

    unit.concentrate.assign(n_conc, std::vector<double>(n_comp, 0.0));

    unit.k_matrix.assign(2, std::vector<double>(n_comp, 0.0));
}

void test_type_a_mass_conservation() {
    Simulator_Parameters params;

    CUnit unit;
    unit.n_outputs = 2;
    unit.unit_type = 0;

    prepare_unit(unit, params);

    unit.feed = {10.0, 15.0, 75.0};

    unit.calculate_outputs(params);

    for (int comp = 0; comp < params.n_components; ++comp) {
        double output = unit.concentrate[0][comp] + unit.tails[comp];

        assert(is_close(unit.feed[comp], output));
    }

    std::cout << "[PASS] Type A mass conservation\n";
}

void test_type_b_mass_conservation() {
    Simulator_Parameters params;

    CUnit unit;
    unit.n_outputs = 3;
    unit.unit_type = 1;

    prepare_unit(unit, params);

    unit.feed = {8.0, 12.0, 80.0};

    unit.calculate_outputs(params);

    for (int comp = 0; comp < params.n_components; ++comp) {
        double output = unit.concentrate[0][comp] + unit.concentrate[1][comp] + unit.tails[comp];

        assert(is_close(unit.feed[comp], output));
    }

    std::cout << "[PASS] Type B mass conservation\n";
}

void test_recovery_bounds() {
    Simulator_Parameters params;

    CUnit unit;
    unit.n_outputs = 3;
    unit.unit_type = 1;

    prepare_unit(unit, params);

    unit.feed = {8.0, 12.0, 80.0};

    double tau = unit.calculate_residence_time(params);

    const auto& k_matrix = params.k_TypeB;

    for (int stream = 0; stream < unit.n_outputs - 1; ++stream) {
        for (int comp = 0; comp < params.n_components; ++comp) {
            double recovery = unit.calculate_recovery(stream, comp, k_matrix, tau);

            assert(recovery >= 0.0);
            assert(recovery <= 1.0);
        }
    }

    std::cout << "[PASS] recovery values remain between 0 and 1\n";
}

void test_zero_feed_outputs_zero() {
    Simulator_Parameters params;

    CUnit unit;
    unit.n_outputs = 2;
    unit.unit_type = 0;

    prepare_unit(unit, params);

    unit.feed = {0.0, 0.0, 0.0};

    unit.calculate_outputs(params);

    for (int comp = 0; comp < params.n_components; ++comp) {
        assert(is_close(unit.concentrate[0][comp], 0.0));
        assert(is_close(unit.tails[comp], 0.0));
    }

    std::cout << "[PASS] zero feed gives zero outputs\n";
}

void test_residence_time_positive() {
    Simulator_Parameters params;

    CUnit unit;
    unit.n_outputs = 2;
    unit.unit_type = 0;

    prepare_unit(unit, params);

    unit.feed = {8.0, 12.0, 80.0};

    double tau = unit.calculate_residence_time(params);

    assert(tau > 0.0);
    assert(std::isfinite(tau));

    std::cout << "[PASS] residence time is positive and finite\n";
}

int main() {
    std::cout << "CUnit tests\n";

    test_type_a_mass_conservation();
    test_type_b_mass_conservation();
    test_recovery_bounds();
    test_zero_feed_outputs_zero();
    test_residence_time_positive();

    std::cout << "All CUnit tests passed.\n";
    return 0;
}
