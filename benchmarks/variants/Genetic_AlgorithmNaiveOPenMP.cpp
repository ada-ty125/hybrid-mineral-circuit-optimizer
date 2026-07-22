#include "Genetic_Algorithm.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <omp.h>

Algorithm_Parameters default_ga_parameters;

namespace GA_Operators {

std::size_t tournament_selection(const std::vector<Individual>& pop, int tournament_size, std::mt19937& rng) {
    std::uniform_int_distribution<std::size_t> dist(0, pop.size() - 1);
    std::size_t best_idx = dist(rng);
    double best_fit = pop[best_idx].fitness;
    for (int i = 1; i < tournament_size; ++i) {
        std::size_t idx = dist(rng);
        if (pop[idx].fitness > best_fit) {
            best_idx = idx; best_fit = pop[idx].fitness;
        }
    }
    return best_idx;
}

void multi_point_crossover(const std::vector<int>& parent1, const std::vector<int>& parent2,
                           std::vector<int>& child1, std::vector<int>& child2, int num_points, std::mt19937& rng) {
    std::size_t length = parent1.size();
    child1 = parent1; child2 = parent2;
    if (length < 2) return;
    std::vector<std::size_t> points(num_points);
    std::uniform_int_distribution<std::size_t> dist(1, length - 1);
    for (int i = 0; i < num_points; ++i) points[i] = dist(rng);
    std::sort(points.begin(), points.end());

    bool swap = false; std::size_t pt_idx = 0;
    for (std::size_t i = 0; i < length; ++i) {
        if (pt_idx < points.size() && i == points[pt_idx]) { swap = !swap; pt_idx++; }
        if (swap) { child1[i] = parent2[i]; child2[i] = parent1[i]; }
    }
}

} // namespace GA_Operators

/**
 * @brief Thread-safe multithreaded engine using isolated random generation states.
 * @details Deconstructs joint-probability penalties into independent structural evaluation slots.
 */
double optimize_hybrid(
    std::span<const int> const fixed_prefix, std::vector<int>& best_discrete_solution,
    std::vector<double>& best_continuous_solution,
    std::function<double(std::span<const int>, std::span<const double>)> fitness_function,
    std::function<bool(std::span<const int>)> validity_checker, Modern_GA_Functions ga_functions,
    Algorithm_Parameters params) {

    std::mt19937 global_rng(std::random_device{}());
    std::size_t discrete_extent = ga_functions.vector_extent(fixed_prefix);
    std::size_t pop_size = params.population_size;
    std::vector<Individual> population(pop_size);

    for (std::size_t i = 0; i < pop_size; ++i) {
        population[i].discrete_vector.resize(discrete_extent);
        do {
            ga_functions.vector_generator(fixed_prefix, population[i].discrete_vector, global_rng);
        } while (!validity_checker(population[i].discrete_vector));
    }

    double global_best_fitness = -1e12;
    unsigned int base_seed = std::random_device{}();

    for (int iter = 0; iter < params.max_iterations; ++iter) {
#pragma omp parallel for
        for (std::size_t i = 0; i < pop_size; ++i) {
            population[i].fitness = fitness_function(population[i].discrete_vector, population[i].continuous_vector);
        }

        std::size_t best_idx = 0;
        double current_gen_best = -1e12;
        for (std::size_t i = 0; i < pop_size; ++i) {
            if (population[i].fitness > current_gen_best) {
                current_gen_best = population[i].fitness;
                best_idx = i;
            }
        }

        if (current_gen_best > global_best_fitness) {
            global_best_fitness = current_gen_best;
            best_discrete_solution = population[best_idx].discrete_vector;
            best_continuous_solution = population[best_idx].continuous_vector;
        }

        std::vector<Individual> next_generation(pop_size);

        // Safe Multithreading: Isolated local RNG pipelines running concurrently
#pragma omp parallel
        {
            int thread_id = omp_get_thread_num();
            std::mt19937 local_rng(base_seed + thread_id + iter);
            std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

#pragma omp for schedule(dynamic)
            for (std::size_t i = 0; i < pop_size; i += 2) {
                std::size_t p1 = GA_Operators::tournament_selection(population, params.tournament_size, local_rng);
                std::size_t p2 = GA_Operators::tournament_selection(population, params.tournament_size, local_rng);

                Individual c1_best = population[p1];
                Individual c2_best = population[p2];
                bool c1_valid = false, c2_valid = false;

                // Decoupled Evaluation Retry Loops: Independently captures legal circuit topologies
                for (int attempt = 0; attempt < 8; ++attempt) {
                    Individual tmp_c1 = population[p1];
                    Individual tmp_c2 = population[p2];

                    if (prob_dist(local_rng) < params.crossover_probability) {
                        GA_Operators::multi_point_crossover(population[p1].discrete_vector, population[p2].discrete_vector,
                                                            tmp_c1.discrete_vector, tmp_c2.discrete_vector, params.num_crossover_points, local_rng);
                    }

                    ga_functions.vector_mutator(fixed_prefix, tmp_c1.discrete_vector, params.mutation_probability, local_rng);
                    ga_functions.vector_mutator(fixed_prefix, tmp_c2.discrete_vector, params.mutation_probability, local_rng);

                    if (!c1_valid && validity_checker(tmp_c1.discrete_vector)) { c1_best = tmp_c1; c1_valid = true; }
                    if (!c2_valid && (i + 1 < pop_size) && validity_checker(tmp_c2.discrete_vector)) { c2_best = tmp_c2; c2_valid = true; }
                    if (c1_valid && (i + 1 >= pop_size || c2_valid)) break;
                }

                next_generation[i] = c1_best;
                if (i + 1 < pop_size) next_generation[i + 1] = c2_best;
            }
        }
        population = std::move(next_generation);
    }
    return global_best_fitness;
}

double optimize_discrete(std::span<const int> const fixed_prefix, std::vector<int>& best_discrete_solution,
                         std::function<double(std::span<const int>)> fitness_function,
                         std::function<bool(std::span<const int>)> validity_checker,
                         Modern_GA_Functions ga_functions, Algorithm_Parameters params) {
    std::vector<double> dummy_continuous;
    auto wrapped_fitness = [&](std::span<const int> d, std::span<const double> c) { return fitness_function(d); };
    return optimize_hybrid(fixed_prefix, best_discrete_solution, dummy_continuous, wrapped_fitness, validity_checker, ga_functions, params);
}

double optimize_continuous(std::span<const int> const fixed_discrete_structure, std::vector<double>& best_continuous_solution,
                            std::function<double(std::span<const int>, std::span<const double>)> fitness_function,
                            Modern_GA_Functions ga_functions, Algorithm_Parameters params) { return 0.0; }
