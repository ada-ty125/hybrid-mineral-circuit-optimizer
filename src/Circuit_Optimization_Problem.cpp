/**
 * @file Circuit_Optimization_Problem.cpp
 * @brief Implements the fixed and swapable circuit optimization workflows.
 *
 * This translation unit bridges circuit-specific vector generation, mutation,
 * validity checks, performance evaluation, and optional visualization output
 * for the command-line optimizer and tuning utilities.
 */

#include "Circuit_Optimization_Problem.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <span>
#include <vector>

#include "RequiredFunctions.h"

namespace {

std::size_t get_extent_fixed(std::span<const int> const fixed_prefix) {
    if (fixed_prefix.size() < 3) return 0;
    int num_inputs = fixed_prefix[0];
    int num_units = fixed_prefix[1];
    std::size_t total_extent = static_cast<std::size_t>(num_inputs);
    for (int i = 0; i < num_units; ++i) {
        total_extent += static_cast<std::size_t>(fixed_prefix[3 + i]);
    }
    return total_extent;
}

void generate_random_vector_fixed(std::span<const int> const fixed_values,
                                  std::span<int> random_values, std::mt19937& rng) {
    if (random_values.empty()) return;

    int num_units = fixed_values[1];
    int num_products = fixed_values[2];
    int max_node_id = num_units + num_products;

    std::uniform_int_distribution<int> feed_dist(0, num_units - 1);
    random_values[0] = feed_dist(rng);

    int current_unit = 0;
    int outputs_assigned_for_current_unit = 0;
    int expected_outputs = fixed_values[3 + current_unit];
    std::vector<int> current_unit_dests;

    for (std::size_t i = 1; i < random_values.size(); ++i) {
        std::vector<int> pool;

        for (int val = 0; val < max_node_id; ++val) {
            if (val == current_unit) continue;
            if (std::find(current_unit_dests.begin(), current_unit_dests.end(), val) !=
                current_unit_dests.end()) {
                continue;
            }

            pool.push_back(val);
        }

        if (!pool.empty()) {
            std::uniform_int_distribution<std::size_t> dist(0, pool.size() - 1);
            random_values[i] = pool[dist(rng)];
        } else {
            random_values[i] = 0;
        }

        current_unit_dests.push_back(random_values[i]);
        outputs_assigned_for_current_unit++;

        if (outputs_assigned_for_current_unit == expected_outputs) {
            current_unit++;
            if (current_unit < num_units) {
                expected_outputs = fixed_values[3 + current_unit];
            }
            outputs_assigned_for_current_unit = 0;
            current_unit_dests.clear();
        }
    }
}

void custom_mutator_fixed(std::span<const int> const fixed_prefix, std::span<int> values,
                          double mutation_rate, std::mt19937& rng) {
    int num_units = fixed_prefix[1];
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (prob_dist(rng) < mutation_rate) {
            int max_val = (i == 0) ? num_units : (num_units + 3);
            std::uniform_int_distribution<int> step_dist(1, max_val - 1);
            int step = step_dist(rng);
            values[i] = (values[i] + step) % max_val;
        }
    }
}

std::size_t get_extent_swapable(std::span<const int> const base_prefix) {
    int num_units = base_prefix[1];
    return num_units + 1 + (num_units * 3);
}

void generate_random_vector_swapable(std::span<const int> const base_prefix,
                                     std::span<int> random_values, std::mt19937& rng) {
    int num_units = base_prefix[1];
    int num_products = base_prefix[2];
    int max_node_id = num_units + num_products;

    if (random_values.size() < get_extent_swapable(base_prefix)) return;

    std::uniform_int_distribution<int> type_dist(2, 3);
    for (int i = 0; i < num_units; ++i) {
        random_values[i] = type_dist(rng);
    }

    std::uniform_int_distribution<int> feed_dist(0, num_units - 1);
    random_values[num_units] = feed_dist(rng);

    int start_idx = num_units + 1;
    for (int u = 0; u < num_units; ++u) {
        std::vector<int> current_unit_dests;

        for (int slot = 0; slot < 3; ++slot) {
            std::vector<int> pool;
            for (int val = 0; val < max_node_id; ++val) {
                if (val == u) continue;
                if (std::find(current_unit_dests.begin(), current_unit_dests.end(), val) !=
                    current_unit_dests.end()) {
                    continue;
                }

                pool.push_back(val);
            }

            if (!pool.empty()) {
                std::uniform_int_distribution<std::size_t> dist(0, pool.size() - 1);
                int chosen_dest = pool[dist(rng)];
                random_values[start_idx + u * 3 + slot] = chosen_dest;
                current_unit_dests.push_back(chosen_dest);
            } else {
                random_values[start_idx + u * 3 + slot] = 0;
            }
        }
    }
}

void custom_mutator_swapable(std::span<const int> const base_prefix, std::span<int> values,
                             double mutation_rate, std::mt19937& rng) {
    int num_units = base_prefix[1];
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (prob_dist(rng) < mutation_rate) {
            if (i < static_cast<std::size_t>(num_units)) {
                values[i] = (values[i] == 2) ? 3 : 2;
            } else if (i == static_cast<std::size_t>(num_units)) {
                std::uniform_int_distribution<int> step_dist(1, num_units - 1);
                values[i] = (values[i] + step_dist(rng)) % num_units;
            } else {
                int max_dest = num_units + 3;
                std::uniform_int_distribution<int> step_dist(1, max_dest - 1);
                values[i] = (values[i] + step_dist(rng)) % max_dest;
            }
        }
    }
}

void print_params(const Algorithm_Parameters& params, const std::string& mode,
                  const std::string& output_image) {
    std::cout << "--- Hyperparameter Configuration ---\n";
    std::cout << "Running Mode:         " << mode << "\n";
    std::cout << "Population Size:      " << params.population_size << "\n";
    std::cout << "Max Iterations:       " << params.max_iterations << "\n";
    std::cout << "Crossover Prob:       " << params.crossover_probability << "\n";
    std::cout << "Mutation Prob:        " << params.mutation_probability << "\n";
    std::cout << "Tournament Size:      " << params.tournament_size << "\n";
    std::cout << "Early Stop Patience:  " << params.early_stop_patience << "\n";
    std::cout << "Crossover Points:     " << params.num_crossover_points << "\n";
    std::cout << "Gaussian Sigma:       " << params.gaussian_sigma << "\n";
    std::cout << "Elite Count:          " << params.elite_count << "\n";
    std::cout << "Output Render Path:   " << output_image << "\n";
    std::cout << "------------------------------------\n";
}

void print_success(const std::string& mode, const std::vector<int>& full_circuit) {
    std::cout << "\n==================================================\n";
    std::cout << "OPTIMIZATION SUCCESSFUL (" << mode << " Case)\n";
    std::cout << "Optimal Circuit Vector: [ ";
    for (int val : full_circuit) std::cout << val << " ";
    std::cout << "]\n==================================================\n\n";
}

}  // namespace

Circuit_Optimization_Result run_circuit_optimization(const Algorithm_Parameters& params,
                                                     const std::string& mode,
                                                     const std::string& output_image,
                                                     bool render_output, bool verbose) {
    Circuit_Optimization_Result result;
    auto start_time = std::chrono::high_resolution_clock::now();

    if (verbose) print_params(params, mode, output_image);

    if (mode == "fixed") {
        std::vector<int> fixed_prefix_layout = {1, 10, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3};

        Modern_GA_Functions ga_functions = {
            .vector_extent = &get_extent_fixed,
            .vector_generator = &generate_random_vector_fixed,
            .vector_mutator = &custom_mutator_fixed,
            .continuous_generator = nullptr,
        };

        auto wrapped_performance = [fixed_prefix_layout](std::span<const int> discrete) {
            std::vector<int> full(fixed_prefix_layout.begin(), fixed_prefix_layout.end());
            full.insert(full.end(), discrete.begin(), discrete.end());
            return circuit_performance(std::span<const int>(full));
        };

        auto wrapped_validity = [fixed_prefix_layout](std::span<const int> discrete) {
            std::vector<int> full(fixed_prefix_layout.begin(), fixed_prefix_layout.end());
            full.insert(full.end(), discrete.begin(), discrete.end());
            return check_validity(std::span<const int>(full));
        };

        result.best_fitness =
            optimize_discrete(fixed_prefix_layout, result.best_discrete_solution,
                              wrapped_performance, wrapped_validity, ga_functions, params);

        if (result.best_fitness > -30000.0 && !result.best_discrete_solution.empty()) {
            result.best_full_circuit = fixed_prefix_layout;
            result.best_full_circuit.insert(result.best_full_circuit.end(),
                                            result.best_discrete_solution.begin(),
                                            result.best_discrete_solution.end());
        }
    } else if (mode == "swapable") {
        std::vector<int> base_prefix = {1, 8, 3};

        Modern_GA_Functions ga_functions = {
            .vector_extent = &get_extent_swapable,
            .vector_generator = &generate_random_vector_swapable,
            .vector_mutator = &custom_mutator_swapable,
            .continuous_generator = nullptr,
        };

        auto pack_swapable_for_eval = [base_prefix](std::span<const int> discrete) {
            std::vector<int> full;
            int num_units = base_prefix[1];

            full.insert(full.end(), base_prefix.begin(), base_prefix.end());
            for (int i = 0; i < num_units; ++i) full.push_back(discrete[i]);

            full.push_back(discrete[num_units]);

            int conn_start = num_units + 1;
            for (int i = 0; i < num_units; ++i) {
                int unit_type = discrete[i];
                for (int j = 0; j < unit_type; ++j) {
                    full.push_back(discrete[conn_start + (i * 3) + j]);
                }
            }
            return full;
        };

        auto wrapped_performance = [&pack_swapable_for_eval](std::span<const int> discrete) {
            std::vector<int> full = pack_swapable_for_eval(discrete);
            return circuit_performance(std::span<const int>(full));
        };

        auto wrapped_validity = [&pack_swapable_for_eval](std::span<const int> discrete) {
            std::vector<int> full = pack_swapable_for_eval(discrete);
            return check_validity(std::span<const int>(full));
        };

        result.best_fitness =
            optimize_discrete(base_prefix, result.best_discrete_solution, wrapped_performance,
                              wrapped_validity, ga_functions, params);

        if (result.best_fitness > -30000.0 && !result.best_discrete_solution.empty()) {
            result.best_full_circuit = pack_swapable_for_eval(result.best_discrete_solution);
        }
    } else {
        if (verbose) {
            std::cerr << "Unknown mode: " << mode << ". Use fixed or swapable.\n";
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    result.runtime_seconds = elapsed.count();
    result.success = result.best_fitness > -30000.0 && !result.best_full_circuit.empty();

    if (verbose) {
        if (result.success) {
            print_success(mode, result.best_full_circuit);
            if (render_output && !output_image.empty()) {
                plot_span(std::span<const int>(result.best_full_circuit), output_image.c_str());
            }
        }
        std::cout << "Optimization runtime: " << result.runtime_seconds << " seconds\n";
        std::cout << "Best performance score: " << result.best_fitness << "\n";
    } else if (render_output && result.success && !output_image.empty()) {
        plot_span(std::span<const int>(result.best_full_circuit), output_image.c_str());
    }

    return result;
}
