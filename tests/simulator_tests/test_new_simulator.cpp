// tests/simulator_tests/test_simulator_refactor.cpp

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "CCircuit.h"
#include "CSimulator.h"

void test_valid_initialise() {
    std::vector<int> circuit_vector = {1, 2, 3, 2, 2, 0, 1, 2, 3, 4};

    Circuit circuit;
    assert(circuit.initialise(circuit_vector));

    assert(circuit.num_inputs() == 1);
    assert(circuit.num_units() == 2);
    assert(circuit.num_products() == 3);
    assert(circuit.feed_dest() == 0);
}

void test_invalid_self_recycle_rejected() {
    std::vector<int> circuit_vector = {1, 2, 3, 2, 2, 0, 0, 2,  // unit 0 sends output to itself
                                       3, 4};

    Circuit circuit;
    assert(!circuit.initialise(circuit_vector));
}

void test_invalid_duplicate_outputs_rejected() {
    std::vector<int> circuit_vector = {1, 2, 3, 2, 2, 0, 1, 1,  // duplicate outputs from unit 0
                                       3, 4};

    Circuit circuit;
    assert(!circuit.initialise(circuit_vector));
}

void test_invalid_product_id_rejected() {
    std::vector<int> circuit_vector = {1, 2, 3, 2, 2, 0, 1, 99,  // invalid destination
                                       3, 4};

    Circuit circuit;
    assert(!circuit.initialise(circuit_vector));
}

void test_reachability_from_feed() {
    std::vector<int> circuit_vector = {1, 3, 3, 2, 2, 2, 0, 1, 3, 2, 4, 3, 4};

    Circuit circuit;
    assert(circuit.initialise(circuit_vector));

    auto reachable = circuit.units_reachable_from_feed();

    assert(reachable.size() == 3);
    assert(reachable[0]);
    assert(reachable[1]);
    assert(reachable[2]);
}

void test_simulator_returns_finite_score() {
    std::vector<int> circuit_vector = {1, 2, 3, 2, 2, 0, 1, 2, 3, 4};

    Circuit circuit;
    assert(circuit.initialise(circuit_vector));

    double score = CSimulator::evaluate(circuit);

    assert(std::isfinite(score));
}

void test_circuit_performance_span_wrapper() {
    std::vector<int> circuit_vector = {1, 2, 3, 2, 2, 0, 1, 2, 3, 4};

    double score = circuit_performance(std::span<const int>(circuit_vector));

    assert(std::isfinite(score));
}

void test_example_5_unit_case() {
    std::vector<int> circuit_vector = {1, 5, 3, 2, 2, 2, 2, 3, 3, 2, 7, 2, 3, 4, 1, 2, 0, 5, 6, 2};

    Circuit circuit;

    assert(circuit.initialise(circuit_vector));

    assert(circuit.num_inputs() == 1);
    assert(circuit.num_units() == 5);
    assert(circuit.num_products() == 3);
    assert(circuit.feed_dest() == 3);

    double score = CSimulator::evaluate(circuit);

    std::cout << "Example 5-unit score = " << score << "\n";

    assert(std::isfinite(score));

    // Expected value from published example:
    assert(std::abs(score - 366) < 1.0);

    std::cout << "Example 5-unit case passed.\n";
}

int main() {
    test_valid_initialise();
    test_invalid_self_recycle_rejected();
    test_invalid_duplicate_outputs_rejected();
    test_invalid_product_id_rejected();
    test_reachability_from_feed();
    test_simulator_returns_finite_score();
    test_circuit_performance_span_wrapper();
    test_example_5_unit_case();

    std::cout << "All simulator refactor tests passed.\n";
    return 0;
}