/**
 * @file speed_checker.cpp
 * @brief Measures repeated circuit-performance evaluation throughput.
 *
 * The tool loads circuit vectors from test data and repeatedly evaluates them
 * to provide a simple simulator speed regression check.
 */

#include <iostream>
#include <chrono>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include "RequiredFunctions.h"

std::vector<std::vector<int>> read_circuit_vectors(const std::string& filename) {
    std::vector<std::vector<int>> circuits;
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return circuits;
    }

    int num_inputs = 0;
    int num_units = 0;
    int num_products = 0;
    while (infile >> num_inputs >> num_units >> num_products) {
        std::vector<int> output_counts(static_cast<std::size_t>(num_units), 0);
        std::size_t total_outputs = 0;
        for (int unit = 0; unit < num_units; ++unit) {
            infile >> output_counts[static_cast<std::size_t>(unit)];
            total_outputs +=
                static_cast<std::size_t>(output_counts[static_cast<std::size_t>(unit)]);
        }

        std::vector<int> connections(static_cast<std::size_t>(num_inputs) + total_outputs, 0);
        for (int& connection : connections) {
            infile >> connection;
        }

        std::vector<int> circuit;
        circuit.reserve(3 + output_counts.size() + 1 + total_outputs);
        circuit.push_back(num_inputs);
        circuit.push_back(num_units);
        circuit.push_back(num_products);
        circuit.insert(circuit.end(), output_counts.begin(), output_counts.end());
        circuit.push_back(connections.front());
        circuit.insert(circuit.end(), connections.begin() + num_inputs, connections.end());
        circuits.push_back(circuit);
    }

    return circuits;
}

int main(int argc, char* argv[]) {
    int num_iterations = 1000;  // Default number of iterations
    if (argc > 1) {
        num_iterations = std::stoi(argv[1]);
    }

    std::vector<std::vector<int>> test_circuits = read_circuit_vectors("data/test_graphs.txt");
    if (test_circuits.empty()) {
        return 1;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    double checksum = 0.0;
    for (int ctr = 0; ctr < num_iterations; ctr++) {
        const auto& circuit = test_circuits[static_cast<std::size_t>(ctr) % test_circuits.size()];
        checksum += circuit_performance(std::span<const int>(circuit));
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_seconds = end_time - start_time;
    std::cout << "Elapsed time for " << num_iterations << " iterations: " << elapsed_seconds.count()
              << " seconds" << std::endl;
    std::cout << "Checksum: " << checksum << std::endl;

    return 0;
}
