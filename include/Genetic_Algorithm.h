#pragma once

#include <span>
#include <vector>
#include <functional>
#include <random>
#include "RequiredFunctions.h"

struct Algorithm_Parameters {
    int max_iterations = 280;
    int population_size = 1000;
    double crossover_probability = 0.75;
    double mutation_probability = 0.15;
    int early_stop_patience = 100;
    int tournament_size = 2;
    int num_crossover_points = 4;
    double gaussian_sigma = 0.05;
    int elite_count = 1;
};

extern Algorithm_Parameters default_ga_parameters;

struct Individual {
    std::vector<int> discrete_vector;
    std::vector<double> continuous_vector;
    double fitness;
};

struct Modern_GA_Functions {
    std::function<std::size_t(std::span<const int>)> vector_extent;
    std::function<void(std::span<const int>, std::span<int>, std::mt19937&)> vector_generator;
    std::function<void(std::span<const int>, std::span<int>, double, std::mt19937&)> vector_mutator;
    std::function<void(std::span<double>, std::mt19937&)> continuous_generator;
};

double optimize_discrete(std::span<const int> const fixed_prefix,
                         std::vector<int>& best_discrete_solution,
                         std::function<double(std::span<const int>)> fitness_function,
                         std::function<bool(std::span<const int>)> validity_checker,
                         Modern_GA_Functions ga_functions,
                         Algorithm_Parameters params = default_ga_parameters);

double optimize_continuous(
    std::span<const int> const fixed_discrete_structure,
    std::vector<double>& best_continuous_solution,
    std::function<double(std::span<const int>, std::span<const double>)> fitness_function,
    Modern_GA_Functions ga_functions, Algorithm_Parameters params = default_ga_parameters);

double optimize_hybrid(
    std::span<const int> const fixed_prefix, std::vector<int>& best_discrete_solution,
    std::vector<double>& best_continuous_solution,
    std::function<double(std::span<const int>, std::span<const double>)> fitness_function,
    std::function<bool(std::span<const int>)> validity_checker, Modern_GA_Functions ga_functions,
    Algorithm_Parameters params = default_ga_parameters);