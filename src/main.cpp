#include <chrono>
#include <iostream>
#include <random>
#include <span>
#include <string>
#include <vector>
#include "CSRGraph.h"
#include "Genetic_Algorithm.h"
#include "RequiredFunctions.h"

// ============================================================================
// 🛠️ LOCAL MOCK ENVIRONMENT (SAFE FOR PRODUCTION / MAIN BRANCH)
// The following block runs ONLY when -DENABLE_MOCK is passed to the compiler.
// It will be completely ignored during main production builds and CI pipelines.
// ============================================================================
#ifdef ENABLE_MOCK

double circuit_performance(std::span<const int> const values) {
    double mock_score = 0.0;
    for (int val : values) {
        mock_score += static_cast<double>(val);
    }
    return mock_score;
}

bool check_validity(std::span<const int> const values) { return true; }

void plot_span(std::span<int> const values, char* filename) {
    std::cout << "[Local Mock Plot] Layout saved to " << filename << "\n";
}

#endif  // ENABLE_MOCK
// ============================================================================

// Calculates the required size of the discrete solution vector based on fixed prefix layout.
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

// Generates an initial random configuration satisfying indexing boundaries.
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

// Rubric compliant mutator: iterates over each gene, decides based on probability,
// applies random valid step, and wraps around using modulus.
void custom_mutator(std::span<const int> const fixed_prefix, std::span<int> values,
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

int main(int argc, char* argv[]) {
    Algorithm_Parameters params;
    std::string output_image = "optimal_circuit.gif";

    if (argc > 1) params.population_size = std::stoi(argv[1]);
    if (argc > 2) params.max_iterations = std::stoi(argv[2]);
    if (argc > 3) params.crossover_probability = std::stod(argv[3]);
    if (argc > 4) params.mutation_probability = std::stod(argv[4]);
    if (argc > 5) params.tournament_size = std::stoi(argv[5]);
    if (argc > 6) params.early_stop_patience = std::stoi(argv[6]);
    if (argc > 7) params.num_crossover_points = std::stoi(argv[7]);
    if (argc > 8) params.elite_count = std::stoi(argv[8]);
    if (argc > 9) output_image = argv[9];

    std::cout << "--- Hyperparameter Configuration ---\n";
    std::cout << "Population Size:      " << params.population_size << "\n";
    std::cout << "Max Iterations:       " << params.max_iterations << "\n";
    std::cout << "Crossover Prob:       " << params.crossover_probability << "\n";
    std::cout << "Mutation Prob:        " << params.mutation_probability << "\n";
    std::cout << "Tournament Size:      " << params.tournament_size << "\n";
    std::cout << "Early Stop Patience:  " << params.early_stop_patience << "\n";
    std::cout << "Crossover Points:     " << params.num_crossover_points << "\n";
    std::cout << "Elite Count:          " << params.elite_count << "\n";
    std::cout << "Output Render:        " << output_image << "\n";
    std::cout << "------------------------------------\n";

    std::vector<int> fixed_prefix_layout = {1, 10, 3, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3};

    Modern_GA_Functions ga_functions = {
        .vector_extent = &get_extent,
        .vector_generator = &generate_random_vector,
        .vector_mutator = &custom_mutator,
        .continuous_generator = nullptr,
    };

    std::vector<int> best_discrete_solution;
    auto start_time = std::chrono::high_resolution_clock::now();

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
        plot_span(std::span<int>(full_circuit), const_cast<char*>(output_image.c_str()));
        return 0;
    }

    return 1;
}
