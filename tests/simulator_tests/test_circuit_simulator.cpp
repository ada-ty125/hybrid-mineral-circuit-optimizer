/**
 * @file test_circuit_simulator.cpp
 * @brief Regression test for graph-based circuit performance evaluation.
 *
 * The test builds a small CSR graph, evaluates it through the simulator, and
 * checks the score against a fixed reference value.
 */

#include <cmath>
#include <iostream>

#include "ESEGraph.h"
#include "CSimulator.h"

using namespace ESE;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    GraphSizes sizes(1, 2, 3);
    offset_t output_index[] = {1, 3, 6};
    input_node_id inputs[] = {0, 1, 2, 0, 2, 3};

    CSRGraph graph(sizes, output_index, inputs);

    const double expected = -39297.985467447535;
    const double tolerance = 1.0e-2;

    std::cout << "circuit_performance(graph) close to " << expected << ":\n";
    Simulator_Parameters params;
    double result = circuit_performance(graph, params);
    std::cout << "circuit_performance(graph) = " << result << "\n";

    if (std::fabs(result - expected) < tolerance) {
        std::cout << "pass\n";
    } else {
        std::cout << "fail\n";
        return 1;
    }
}
