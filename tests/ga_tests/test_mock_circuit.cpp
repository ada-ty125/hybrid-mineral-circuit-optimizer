/**
 * @file test_mock_circuit.cpp
 * @brief Tests the GA against a deterministic mock circuit objective.
 *
 * The mock objective provides a cheap target-vector fitness function so the GA
 * can be checked without invoking the full circuit simulator.
 */

#include <iostream>
#include <span>
#include <vector>
#include <cmath>
#include <random>
#include <functional>
#include "Genetic_Algorithm.h"

std::vector<int> target_mock_answer;

double mock_performance(std::span<const int> const values) {
    double performance = 0.0;
    for (std::size_t i = 0; i < values.size(); i++) {
        performance += (20.0 - std::abs(values[i] - target_mock_answer[i])) * 100.0;
    }
    return performance;
}

bool mock_validity_checker(std::span<const int> const values) { return true; }

std::size_t get_mock_extent(std::span<const int> const fixed_prefix) {
    int n_units = fixed_prefix[0];
    return 3 * n_units + 1;
}

void generate_random_mock_vector(std::span<const int> const fixed_prefix, std::span<int> values,
                                 std::mt19937& rng) {
    int n_units = fixed_prefix[0];
    int max_val = n_units + 3;
    std::uniform_int_distribution<int> dist(0, max_val - 1);
    for (std::size_t i = 0; i < values.size(); i++) {
        values[i] = dist(rng);
    }
}

void mutate_mock_vector(std::span<const int> const fixed_prefix, std::span<int> values,
                        std::size_t max_mutations, std::mt19937& rng) {
    int n_units = fixed_prefix[0];
    int max_val = n_units + 3;
    std::uniform_int_distribution<std::size_t> idx_dist(0, values.size() - 1);
    std::uniform_int_distribution<int> step_dist(1, max_val - 1);
    for (std::size_t i = 0; i < max_mutations; i++) {
        std::size_t idx = idx_dist(rng);
        int step = step_dist(rng);
        values[idx] = (values[idx] + step) % max_val;
    }
}

int main() {
    int fixed[] = {10};
    std::size_t extent = get_mock_extent(std::span<const int>(fixed));
    target_mock_answer.resize(extent);
    for (std::size_t i = 0; i < extent; i++) {
        target_mock_answer[i] = (i % (fixed[0] + 3));
    }
    std::cout << "Starting Genetic Algorithm Mock Test" << std::endl;
    Algorithm_Parameters params{.max_iterations = 500,
                                .population_size = 100,
                                .crossover_probability = 0.85,
                                .mutation_probability = 0.05,
                                .max_mutations = 3,
                                .early_stop_patience = 50,
                                .tournament_size = 4,
                                .num_crossover_points = 2};
    Modern_GA_Functions mock_ga_functions{.vector_extent = &get_mock_extent,
                                          .vector_generator = &generate_random_mock_vector,
                                          .vector_mutator = &mutate_mock_vector,
                                          .continuous_generator = nullptr};
    std::vector<int> best_solution;
    double best_fitness =
        optimize_discrete(std::span<const int>(fixed), best_solution, &mock_performance,
                          &mock_validity_checker, mock_ga_functions, params);
    std::cout << "Best Mock Performance: " << best_fitness << std::endl;
    std::cout << "Maximum Possible Performance: " << (20.0 * 100.0 * extent) << std::endl;
    return 0;
}
