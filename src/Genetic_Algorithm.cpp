#include "Genetic_Algorithm.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <omp.h>

Algorithm_Parameters default_ga_parameters;

namespace GA_Operators {

std::size_t tournament_selection(const std::vector<Individual>& pop, int tournament_size,
                                 std::mt19937& rng) {
    std::uniform_int_distribution<std::size_t> dist(0, pop.size() - 1);
    std::size_t best_idx = dist(rng);
    double best_fit = pop[best_idx].fitness;
    for (int i = 1; i < tournament_size; ++i) {
        std::size_t idx = dist(rng);
        if (pop[idx].fitness > best_fit) {
            best_idx = idx;
            best_fit = pop[idx].fitness;
        }
    }
    return best_idx;
}

void multi_point_crossover(const std::vector<int>& parent1, const std::vector<int>& parent2,
                           std::vector<int>& child1, std::vector<int>& child2, int num_points,
                           std::mt19937& rng) {
    std::size_t length = parent1.size();
    child1 = parent1;
    child2 = parent2;
    if (length < 2) return;
    std::vector<std::size_t> points(num_points);
    std::uniform_int_distribution<std::size_t> dist(1, length - 1);
    for (int i = 0; i < num_points; ++i) {
        points[i] = dist(rng);
    }
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    bool swap = false;
    std::size_t pt_idx = 0;
    for (std::size_t i = 0; i < length; ++i) {
        if (pt_idx < points.size() && i == points[pt_idx]) {
            swap = !swap;
            pt_idx++;
        }
        if (swap) {
            child1[i] = parent2[i];
            child2[i] = parent1[i];
        }
    }
}

void linear_crossover(const std::vector<double>& parent1, const std::vector<double>& parent2,
                      std::vector<double>& child1, std::vector<double>& child2, std::mt19937& rng) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (std::size_t i = 0; i < parent1.size(); ++i) {
        double alpha = dist(rng);
        child1[i] = alpha * parent1[i] + (1.0 - alpha) * parent2[i];
        child2[i] = alpha * parent2[i] + (1.0 - alpha) * parent1[i];
    }
}

void gaussian_mutation(std::vector<double>& vec, double mutation_rate, double sigma,
                       std::mt19937& rng) {
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    std::normal_distribution<double> gauss_dist(0.0, sigma);
    for (double& val : vec) {
        if (prob_dist(rng) < mutation_rate) {
            val += gauss_dist(rng);
            val = std::clamp(val, 0.0, 1.0);
        }
    }
}
}  // namespace GA_Operators

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
    for (std::size_t i = 0; i < pop_size; ++i) {
        population[i].discrete_vector.resize(discrete_extent);
        population[i].continuous_vector.resize(continuous_extent);
        do {
            ga_functions.vector_generator(fixed_prefix, population[i].discrete_vector, global_rng);
        } while (!validity_checker(population[i].discrete_vector));
        if (ga_functions.continuous_generator) {
            ga_functions.continuous_generator(population[i].continuous_vector, global_rng);
        }
    }
    double global_best_fitness = -1e12;
    int patience_counter = 0;
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    for (int iter = 0; iter < params.max_iterations; ++iter) {
#pragma omp parallel for
        for (std::size_t i = 0; i < pop_size; ++i) {
            if (validity_checker(population[i].discrete_vector)) {
                population[i].fitness = fitness_function(population[i].discrete_vector,
                                                         population[i].continuous_vector);
            } else {
                population[i].fitness = -1e12;
            }
        }
        std::size_t best_idx = 0;
        double current_gen_best = -1e12;
        for (std::size_t i = 0; i < pop_size; ++i) {
            if (population[i].fitness > current_gen_best) {
                current_gen_best = population[i].fitness;
                best_idx = i;
            }
        }
        if (current_gen_best > global_best_fitness + 1e-6) {
            global_best_fitness = current_gen_best;
            best_discrete_solution = population[best_idx].discrete_vector;
            best_continuous_solution = population[best_idx].continuous_vector;
            patience_counter = 0;
        } else {
            patience_counter++;
        }
        if (patience_counter >= params.early_stop_patience) {
            std::cout << "[GA] Early stopping triggered at generation " << iter << "\n";
            break;
        }
        std::vector<Individual> next_generation(pop_size);
        next_generation[0] = population[best_idx];
        std::size_t child_count = 1;
        while (child_count < pop_size) {
            std::size_t p1 =
                GA_Operators::tournament_selection(population, params.tournament_size, global_rng);
            std::size_t p2 =
                GA_Operators::tournament_selection(population, params.tournament_size, global_rng);
            Individual c1 = population[p1];
            Individual c2 = population[p2];
            if (prob_dist(global_rng) < params.crossover_probability) {
                GA_Operators::multi_point_crossover(population[p1].discrete_vector,
                                                    population[p2].discrete_vector,
                                                    c1.discrete_vector, c2.discrete_vector,
                                                    params.num_crossover_points, global_rng);
                if (continuous_extent > 0) {
                    GA_Operators::linear_crossover(
                        population[p1].continuous_vector, population[p2].continuous_vector,
                        c1.continuous_vector, c2.continuous_vector, global_rng);
                }
            }
            if (prob_dist(global_rng) < params.mutation_probability) {
                ga_functions.vector_mutator(fixed_prefix, c1.discrete_vector, params.max_mutations,
                                            global_rng);
            }
            if (continuous_extent > 0) {
                GA_Operators::gaussian_mutation(c1.continuous_vector, params.mutation_probability,
                                                params.gaussian_sigma, global_rng);
            }
            if (validity_checker(c1.discrete_vector) && child_count < pop_size) {
                next_generation[child_count++] = c1;
            }
            if (prob_dist(global_rng) < params.mutation_probability) {
                ga_functions.vector_mutator(fixed_prefix, c2.discrete_vector, params.max_mutations,
                                            global_rng);
            }
            if (continuous_extent > 0) {
                GA_Operators::gaussian_mutation(c2.continuous_vector, params.mutation_probability,
                                                params.gaussian_sigma, global_rng);
            }
            if (validity_checker(c2.discrete_vector) && child_count < pop_size) {
                next_generation[child_count++] = c2;
            }
        }
        population = std::move(next_generation);
    }
    return global_best_fitness;
}

double optimize_discrete(std::span<const int> const fixed_prefix,
                         std::vector<int>& best_discrete_solution,
                         std::function<double(std::span<const int>)> fitness_function,
                         std::function<bool(std::span<const int>)> validity_checker,
                         Modern_GA_Functions ga_functions, Algorithm_Parameters params) {
    std::vector<double> dummy_continuous;
    auto wrapped_fitness = [&](std::span<const int> discrete, std::span<const double> continuous) {
        return fitness_function(discrete);
    };
    return optimize_hybrid(fixed_prefix, best_discrete_solution, dummy_continuous, wrapped_fitness,
                           validity_checker, ga_functions, params);
}

double optimize_continuous(
    std::span<const int> const fixed_discrete_structure,
    std::vector<double>& best_continuous_solution,
    std::function<double(std::span<const int>, std::span<const double>)> fitness_function,
    Modern_GA_Functions ga_functions, Algorithm_Parameters params) {
    auto dummy_validity = [](std::span<const int> vec) { return true; };
    std::vector<int> dummy_discrete(fixed_discrete_structure.begin(),
                                    fixed_discrete_structure.end());
    Algorithm_Parameters modified_params = params;
    modified_params.crossover_probability = 1.0;
    auto frozen_generator = [fixed_discrete_structure](std::span<const int> prefix,
                                                       std::span<int> values, std::mt19937& rng) {
        std::copy(fixed_discrete_structure.begin(), fixed_discrete_structure.end(), values.begin());
    };
    auto frozen_mutator = [](std::span<const int> prefix, std::span<int> values,
                             std::size_t max_mutations, std::mt19937& rng) {};
    auto frozen_extent = [fixed_discrete_structure](std::span<const int> prefix) {
        return fixed_discrete_structure.size();
    };
    Modern_GA_Functions modular_ga_functions;
    modular_ga_functions.vector_generator = frozen_generator;
    modular_ga_functions.vector_mutator = frozen_mutator;
    modular_ga_functions.vector_extent = frozen_extent;
    modular_ga_functions.continuous_generator = ga_functions.continuous_generator;
    return optimize_hybrid(fixed_discrete_structure, dummy_discrete, best_continuous_solution,
                           fitness_function, dummy_validity, modular_ga_functions, modified_params);
}

void default_generator(std::span<const int> const, std::span<int>, RandomNumberGenerator&) {}
void default_mutator(std::span<const int> const, std::span<int>, std::size_t,
                     RandomNumberGenerator&) {}
void default_crossover(std::span<const int> const, std::span<const int> const,
                       std::span<const int> const, std::span<std::span<int>>,
                       RandomNumberGenerator&) {}
double default_similarity(std::span<const int> const, std::span<const int> const,
                          std::span<const int> const) {
    return 0.0;
}
BaseResult<std::span<int>> optimize_span(std::span<const int> const fixed_prefix,
                                         double (*)(const std::span<const int>),
                                         bool (*)(const std::span<const int>), GA_Functions) {
    int* empty_data = new int[fixed_prefix.size()];
    std::copy(fixed_prefix.begin(), fixed_prefix.end(), empty_data);
    return BaseResult<std::span<int>>{std::span<int>(empty_data, fixed_prefix.size()), 0.0};
}