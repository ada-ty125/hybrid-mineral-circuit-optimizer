#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <span>
#include <random>
#include "Genetic_Algorithm.h"
#include "RequiredFunctions.h"

std::size_t get_extent(std::span<const int> const fixed_prefix) {
    if (fixed_prefix.size() < 3) return 0;
    int num_inputs = fixed_prefix[0];
    int num_units = fixed_prefix[1];
    std::size_t total_extent = static_cast<std::size_t>(num_inputs);
    for (int i = 0; i < num_units; ++i) {
        total_extent += static_cast<std::size_t>(fixed_prefix[3 + i]);
    }
    return total_extent;
}

void generate_random_vector(std::span<const int> const fixed_values, std::span<int> random_values,
                            std::mt19937& rng) {
    int num_units = fixed_values[1];
    int max_destination_val = num_units + 3;
    std::uniform_int_distribution<int> dest_dist(0, max_destination_val - 1);
    std::uniform_int_distribution<int> feed_dist(0, num_units - 1);
    if (!random_values.empty()) {
        random_values[0] = feed_dist(rng);
        for (std::size_t i = 1; i < random_values.size(); ++i) {
            random_values[i] = dest_dist(rng);
        }
    }
}

void custom_mutator(std::span<const int> const fixed_prefix, std::span<int> values,
                    std::size_t max_mutations, std::mt19937& rng) {
    int num_units = fixed_prefix[1];
    int max_val = num_units + 3;
    std::uniform_int_distribution<std::size_t> idx_dist(0, values.size() - 1);
    std::uniform_int_distribution<int> step_dist(1, max_val - 1);
    for (std::size_t i = 0; i < max_mutations; ++i) {
        std::size_t idx = idx_dist(rng);
        int step = step_dist(rng);
        values[idx] = (values[idx] + step) % max_val;
    }
}

int main(int argc, char* argv[]) {
    int population_size = 300;
    int max_iterations = 1000;
    std::string output_image = "optimal_circuit.png";
    if (argc > 1) population_size = std::stoi(argv[1]);
    if (argc > 2) max_iterations = std::stoi(argv[2]);
    if (argc > 3) output_image = argv[3];
    std::vector<int> fixed_prefix_layout = {1, 10, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3};
    Algorithm_Parameters params;
    params.population_size = population_size;
    params.max_iterations = max_iterations;
    params.early_stop_patience = 100;
    Modern_GA_Functions ga_functions{.vector_extent = &get_extent,
                                     .vector_generator = &generate_random_vector,
                                     .vector_mutator = &custom_mutator,
                                     .continuous_generator = nullptr};
    std::vector<int> best_discrete_solution;
    auto start_time = std::chrono::high_resolution_clock::now();
    auto wrapped_performance = [fixed_prefix_layout](std::span<const int> discrete) {
        std::vector<int> full(fixed_prefix_layout.begin(), fixed_prefix_layout.end());
        full.insert(full.end(), discrete.begin(), discrete.end());
        return circuit_performance(full);
    };
    auto wrapped_validity = [fixed_prefix_layout](std::span<const int> discrete) {
        std::vector<int> full(fixed_prefix_layout.begin(), fixed_prefix_layout.end());
        full.insert(full.end(), discrete.begin(), discrete.end());
        return check_validity(full);
    };
    double best_fitness =
        optimize_discrete(fixed_prefix_layout, best_discrete_solution, wrapped_performance,
                          wrapped_validity, ga_functions, params);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "Optimization runtime: " << elapsed.count() << " seconds\n";
    std::cout << "Best performance score: " << best_fitness << "\n";
    if (best_fitness > -30000.0 && !best_discrete_solution.empty()) {
        std::vector<int> full_circuit(fixed_prefix_layout.begin(), fixed_prefix_layout.end());
        full_circuit.insert(full_circuit.end(), best_discrete_solution.begin(),
                            best_discrete_solution.end());
        plot_span(full_circuit, const_cast<char*>(output_image.c_str()));
        return 0;
    }
    return 1;
}