#include "Genetic_Algorithm.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>

Algorithm_Parameters default_ga_parameters;

namespace GA_Operators {

/**
 * @brief Selects an individual using textbook Roulette Wheel Selection.
 * @details Shifts negative scores relative to the population minimum to build a viable cumulative wheel.
 */
std::size_t roulette_wheel_selection(const std::vector<Individual>& pop, std::mt19937& rng) {
    double min_fit = 1e12;
    for (const auto& ind : pop) {
        if (ind.fitness < min_fit) min_fit = ind.fitness;
    }

    std::vector<double> shifted_fitness(pop.size());
    double total_shifted_fitness = 0.0;
    for (std::size_t i = 0; i < pop.size(); ++i) {
        // Offset negative values to guarantee a strictly positive probability map
        shifted_fitness[i] = pop[i].fitness - min_fit + 1.0;
        total_shifted_fitness += shifted_fitness[i];
    }

    std::uniform_real_distribution<double> dist(0.0, total_shifted_fitness);
    double target = dist(rng);
    double current_sum = 0.0;

    for (std::size_t i = 0; i < pop.size(); ++i) {
        current_sum += shifted_fitness[i];
        if (current_sum >= target) {
            return i;
        }
    }
    return pop.size() - 1;
}

/**
 * @brief Performs standard single-point discrete crossover.
 * @details Splits parents at a singular random index and exchanges trailing gene pools.
 */
void single_point_crossover(const std::vector<int>& parent1, const std::vector<int>& parent2,
                            std::vector<int>& child1, std::vector<int>& child2, std::mt19937& rng) {
    std::size_t length = parent1.size();
    child1 = parent1;
    child2 = parent2;
    if (length < 2) return;

    std::uniform_int_distribution<std::size_t> dist(1, length - 1);
    std::size_t cut_point = dist(rng);

    for (std::size_t i = cut_point; i < length; ++i) {
        child1[i] = parent2[i];
        child2[i] = parent1[i];
    }
}

} // namespace GA_Operators

/**
 * @brief Single-threaded baseline optimizer implementing standard generational replacement.
 */
double optimize_hybrid(
    std::span<const int> const fixed_prefix, std::vector<int>& best_discrete_solution,
    std::vector<double>& best_continuous_solution,
    std::function<double(std::span<const int>, std::span<const double>)> fitness_function,
    std::function<bool(std::span<const int>)> validity_checker, Modern_GA_Functions ga_functions,
    Algorithm_Parameters params) {

    std::mt19937 global_rng(std::random_device{}());
    std::size_t discrete_extent = ga_functions.vector_extent(fixed_prefix);
    std::size_t continuous_extent = best_continuous_solution.size();
    std::size_t pop_size = params.population_size;
    std::vector<Individual> population(pop_size);

    // Naive initialization: loops blindly until valid states emerge
    for (std::size_t i = 0; i < pop_size; ++i) {
        population[i].discrete_vector.resize(discrete_extent);
        population[i].continuous_vector.resize(continuous_extent);
        do {
            ga_functions.vector_generator(fixed_prefix, population[i].discrete_vector, global_rng);
        } while (!validity_checker(population[i].discrete_vector));
    }

    double global_best_fitness = -1e12;
    std::ofstream metrics_log("convergence.csv");

    for (int iter = 0; iter < params.max_iterations; ++iter) {
        // Serial evaluation of individuals sequentially
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

        std::vector<Individual> next_generation;
        next_generation.reserve(pop_size);

        // Generational breeding without retry logic or parallel optimizations
        while (next_generation.size() < pop_size) {
            std::size_t p1 = GA_Operators::roulette_wheel_selection(population, global_rng);
            std::size_t p2 = GA_Operators::roulette_wheel_selection(population, global_rng);

            Individual c1 = population[p1];
            Individual c2 = population[p2];

            std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
            if (prob_dist(global_rng) < params.crossover_probability) {
                GA_Operators::single_point_crossover(population[p1].discrete_vector, population[p2].discrete_vector,
                                                     c1.discrete_vector, c2.discrete_vector, global_rng);
            }

            ga_functions.vector_mutator(fixed_prefix, c1.discrete_vector, params.mutation_probability, global_rng);
            ga_functions.vector_mutator(fixed_prefix, c2.discrete_vector, params.mutation_probability, global_rng);

            // Stale clone assignment if structural boundary conditions are broken
            if (!validity_checker(c1.discrete_vector)) c1 = population[p1];
            if (!validity_checker(c2.discrete_vector)) c2 = population[p2];

            next_generation.push_back(std::move(c1));
            if (next_generation.size() < pop_size) {
                next_generation.push_back(std::move(c2));
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
                            Modern_GA_Functions ga_functions, Algorithm_Parameters params) {
    return 0.0;
}