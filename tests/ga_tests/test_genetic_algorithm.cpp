#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <span>
#include <vector>
#include "Genetic_Algorithm.h"

double TSP_performance(std::span<const int> const values) {
    double x_coords[] = {0, 1, 0, 1, 0.5, 0.5};
    double y_coords[] = {0, 0, 1, 1, 0.5, 1.0};
    double total_distance = 0;
    int n_cities = values[0];
    std::size_t from = n_cities - 1;
    for (auto to : values.subspan(1)) {
        double dx = x_coords[from] - x_coords[to];
        double dy = y_coords[from] - y_coords[to];
        total_distance += std::sqrt(dx * dx + dy * dy);
        from = (from + 1) % n_cities;
    }
    return 1000.0 / (total_distance + 1e-6);
}

bool TSP_validity_checker(std::span<const int> const values) {
    int n_cities = values[0];
    if (n_cities != static_cast<int>(values.size() - 1)) {
        return false;
    }
    std::vector<bool> seen(n_cities, false);
    std::span<const int> cities = values.subspan(1);
    for (auto city : cities) {
        if (city < 0 || city >= n_cities || seen[city]) {
            return false;
        }
        seen[city] = true;
    }
    return true;
}

std::size_t get_TSP_extent(std::span<const int> const fixed_prefix) { return fixed_prefix[0] + 1; }

void generate_random_TSP_vector(std::span<const int> const fixed_prefix, std::span<int> values,
                                std::mt19937& rng) {
    std::size_t n_cities = fixed_prefix[0];
    values[0] = static_cast<int>(n_cities);
    for (std::size_t i = 0; i < n_cities; i++) {
        values[i + 1] = i;
    }
    std::shuffle(values.begin() + 1, values.end(), rng);
}

void mutate_TSP_vector(std::span<const int> const fixed_prefix, std::span<int> values,
                       std::size_t max_mutations, std::mt19937& rng) {
    std::size_t n_cities = fixed_prefix[0];
    std::uniform_int_distribution<std::size_t> dist(1, n_cities);
    for (std::size_t i = 0; i < max_mutations; i++) {
        std::size_t j = dist(rng);
        std::size_t k = dist(rng);
        std::swap(values[j], values[k]);
    }
}

int main() {
    std::cout << "Checking optimum for 6 city TSP problem" << std::endl;
    int fixed[] = {6};
    Modern_GA_Functions ga_functions = {
        .vector_extent = &get_TSP_extent,
        .vector_generator = &generate_random_TSP_vector,
        .vector_mutator = &mutate_TSP_vector,
        .continuous_generator = nullptr,
    };
    std::vector<int> best_tour;
    double best_fitness = optimize_discrete(std::span<const int>(fixed), best_tour,
                                            &TSP_performance, &TSP_validity_checker, ga_functions);
    std::cout << "TSP Optimization Complete. Best Fitness Score: " << best_fitness << std::endl;
    std::cout << "Optimized Tour Sequence: ";
    for (size_t i = 1; i < best_tour.size(); ++i) {
        std::cout << best_tour[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}
