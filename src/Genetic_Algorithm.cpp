#include "Genetic_Algorithm.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>

#ifdef _OPENMP
#include <omp.h>
#endif

Algorithm_Parameters default_ga_parameters;

namespace GA_Operators {

/**
 * @brief Selects a high-performing parent using K-way tournament selection.
 * @details Randomly samples K competitors and rewards the ordinal winner to direct selection
 * pressure.
 */
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

/**
 * @brief Recombines discrete structure chromosomes via multi-point segment swapping.
 * @details Extracts unique cut points to split and alternate localized circuit modules between
 * parents.
 */
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

/**
 * @brief Blends continuous numerical dimensions via parametric linear interpolation.
 */
void linear_crossover(const std::vector<double>& parent1, const std::vector<double>& parent2,
                      std::vector<double>& child1, std::vector<double>& child2, std::mt19937& rng) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (std::size_t i = 0; i < parent1.size(); ++i) {
        double alpha = dist(rng);
        child1[i] = alpha * parent1[i] + (1.0 - alpha) * parent2[i];
        child2[i] = alpha * parent2[i] + (1.0 - alpha) * parent1[i];
    }
}

/**
 * @brief Introduces small Gaussian adjustments to real variables with boundary enforcement.
 */
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

/**
 * @brief Unified optimization core deploying (mu + lambda) replacement and localized retries.
 * @details Implements incremental OpenMP evaluation, incest prevention, and continuous seeding
 * heuristics. If breeding constraints fail, it drops parent cloning and injects fresh valid
 * randomized explorers.
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

    std::normal_distribution<double> noise_dist(0.0, 0.05);
    std::uniform_real_distribution<double> uni_dist(0.0, 1.0);
    std::size_t biased_count = static_cast<std::size_t>(pop_size * 0.25);

    // Initial Seeding Phase: Blends pure random states with industry-informed continuous baselines
    for (std::size_t i = 0; i < pop_size; ++i) {
        population[i].discrete_vector.resize(discrete_extent);
        population[i].continuous_vector.resize(continuous_extent);

        int attempt_count = 0;
        const int MAX_ATTEMPTS = 500;
        bool is_valid = false;

        do {
            ga_functions.vector_generator(fixed_prefix, population[i].discrete_vector, global_rng);
            is_valid = validity_checker(population[i].discrete_vector);
            attempt_count++;

            if (!is_valid && attempt_count >= MAX_ATTEMPTS) {
                std::fill(population[i].discrete_vector.begin(),
                          population[i].discrete_vector.end(), 0);
                break;
            }
        } while (!is_valid);

        if (continuous_extent > 0) {
            if (i < biased_count) {
                for (double& x : population[i].continuous_vector) {
                    x = std::clamp(0.5 + noise_dist(global_rng), 0.0, 1.0);
                }
            } else {
                for (double& x : population[i].continuous_vector) {
                    x = uni_dist(global_rng);
                }
            }
        }
    }

    double global_best_fitness = -1e12;
    int patience_counter = 0;
    unsigned int base_seed = std::random_device{}();

    std::ofstream metrics_log("convergence.csv");
    if (metrics_log.is_open()) {
        metrics_log << "Generation,CurrentBest,GlobalBest\n";
    }

    // Initial baseline grading loop for generation zero
#pragma omp parallel for
    for (std::size_t i = 0; i < pop_size; ++i) {
        population[i].fitness =
            fitness_function(population[i].discrete_vector, population[i].continuous_vector);
    }

    for (int iter = 0; iter < params.max_iterations; ++iter) {
        // Track the current champion across the live array
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

        if (metrics_log.is_open()) {
            metrics_log << iter << "," << current_gen_best << "," << global_best_fitness << "\n";
        }

        if (patience_counter >= params.early_stop_patience) {
            std::cout << "[GA] Early stopping triggered at generation " << iter << "\n";
            break;
        }

        // Mutation Rate Shocking: Shifts exploration gears when optimization stalls
        double current_mut_prob = params.mutation_probability;
        if (patience_counter > params.early_stop_patience / 2) {
            current_mut_prob = std::min(1.0, current_mut_prob * 2.5);
        }

        // Generate offspring equal to population size to execute full pool competition
        std::vector<Individual> children(pop_size);

// Parallel Generation Flow: Thread-safe localized pipelines running concurrently
#pragma omp parallel
        {
            int thread_id = 0;
#ifdef _OPENMP
            thread_id = omp_get_thread_num();
#endif
            std::mt19937 local_rng(base_seed + thread_id + iter);
            std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

#pragma omp for schedule(dynamic)
            for (std::size_t i = 0; i < pop_size; i += 2) {
                std::size_t p1 = GA_Operators::tournament_selection(
                    population, params.tournament_size, local_rng);
                std::size_t p2 = GA_Operators::tournament_selection(
                    population, params.tournament_size, local_rng);

                for (int retry = 0; retry < 5 && p1 == p2; ++retry) {
                    p2 = GA_Operators::tournament_selection(population, params.tournament_size,
                                                            local_rng);
                }

                Individual c1_best = population[p1];
                Individual c2_best = population[p2];
                bool c1_any_valid = false;
                bool c2_any_valid = false;

                for (int attempt = 0; attempt < 8; ++attempt) {
                    Individual tmp_c1 = population[p1];
                    Individual tmp_c2 = population[p2];

                    if (prob_dist(local_rng) < params.crossover_probability) {
                        GA_Operators::multi_point_crossover(
                            population[p1].discrete_vector, population[p2].discrete_vector,
                            tmp_c1.discrete_vector, tmp_c2.discrete_vector,
                            params.num_crossover_points, local_rng);
                        if (continuous_extent > 0) {
                            GA_Operators::linear_crossover(
                                population[p1].continuous_vector, population[p2].continuous_vector,
                                tmp_c1.continuous_vector, tmp_c2.continuous_vector, local_rng);
                        }
                    }

                    ga_functions.vector_mutator(fixed_prefix, tmp_c1.discrete_vector,
                                                current_mut_prob, local_rng);
                    if (continuous_extent > 0) {
                        GA_Operators::gaussian_mutation(tmp_c1.continuous_vector, current_mut_prob,
                                                        params.gaussian_sigma, local_rng);
                    }

                    ga_functions.vector_mutator(fixed_prefix, tmp_c2.discrete_vector,
                                                current_mut_prob, local_rng);
                    if (continuous_extent > 0) {
                        GA_Operators::gaussian_mutation(tmp_c2.continuous_vector, current_mut_prob,
                                                        params.gaussian_sigma, local_rng);
                    }

                    if (!c1_any_valid && validity_checker(tmp_c1.discrete_vector)) {
                        c1_best = tmp_c1;
                        c1_any_valid = true;
                    }
                    if (!c2_any_valid && (i + 1 < pop_size) &&
                        validity_checker(tmp_c2.discrete_vector)) {
                        c2_best = tmp_c2;
                        c2_any_valid = true;
                    }

                    if (c1_any_valid && (i + 1 >= pop_size || c2_any_valid)) {
                        break;
                    }
                }

                // Explorer Injection: Enforces unique structural trials over parent clones
                if (!c1_any_valid) {
                    int exp_attempts1 = 0;
                    bool exp1_valid = false;
                    do {
                        ga_functions.vector_generator(fixed_prefix, c1_best.discrete_vector,
                                                      local_rng);
                        exp1_valid = validity_checker(c1_best.discrete_vector);
                        exp_attempts1++;
                        if (!exp1_valid && exp_attempts1 >= 500) {
                            std::fill(c1_best.discrete_vector.begin(),
                                      c1_best.discrete_vector.end(), 0);
                            break;
                        }
                    } while (!exp1_valid);

                    if (continuous_extent > 0) {
                        for (double& x : c1_best.continuous_vector) x = uni_dist(local_rng);
                    }
                }

                if (!c2_any_valid && (i + 1 < pop_size)) {
                    int exp_attempts2 = 0;
                    bool exp2_valid = false;
                    do {
                        ga_functions.vector_generator(fixed_prefix, c2_best.discrete_vector,
                                                      local_rng);
                        exp2_valid = validity_checker(c2_best.discrete_vector);
                        exp_attempts2++;
                        if (!exp2_valid && exp_attempts2 >= 500) {
                            std::fill(c2_best.discrete_vector.begin(),
                                      c2_best.discrete_vector.end(), 0);
                            break;
                        }
                    } while (!exp2_valid);

                    if (continuous_extent > 0) {
                        for (double& x : c2_best.continuous_vector) x = uni_dist(local_rng);
                    }
                }

                children[i] = c1_best;
                if (i + 1 < pop_size) {
                    children[i + 1] = c2_best;
                }
            }
        }

        // Incremental Evaluation: Only evaluates the fresh offspring via OpenMP
#pragma omp parallel for
        for (std::size_t i = 0; i < pop_size; ++i) {
            children[i].fitness =
                fitness_function(children[i].discrete_vector, children[i].continuous_vector);
        }

        // LETHAL CRITICAL RETENTION: Implements the last-year (mu + lambda) survival scheme
        // Merges parents and kids into a double-sized pool, then selects the absolute survival
        // front.
        population.insert(population.end(), std::make_move_iterator(children.begin()),
                          std::make_move_iterator(children.end()));
        std::sort(population.begin(), population.end(),
                  [](const Individual& a, const Individual& b) { return a.fitness > b.fitness; });
        population.resize(pop_size);
    }

    if (metrics_log.is_open()) metrics_log.close();
    return global_best_fitness;
}

/**
 * @brief Discrete facade layer translating discrete parameters into the integrated solver.
 */
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

/**
 * @brief Continuous facade executing real-parameter optimization on frozen discrete backgrounds.
 */
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
                             double mutation_rate, std::mt19937& rng) {};
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