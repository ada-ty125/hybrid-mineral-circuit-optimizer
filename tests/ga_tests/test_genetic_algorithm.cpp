#include <iostream>
#include <span>

#include "Genetic_Algorithm.h"

double TSP_performance(std::span<const int> const values) {
    // This implements a performance function for the traveling salesman problem,
    // where the vector represents a tour of n cities.
    // The optimum solution should have the minimum total distance.

    // x, y for the cities (first is root node/sink node, which is the start and end point of the tour)

    double x_coords[] = {0, 1, 0, 1, 0.5, 0.5};
    double y_coords[] = {0, 0, 1, 1, 0.5, 1.0};

    double total_distance = 0;

    int n_cities = values[0]; // first element of the vector is the number of cities    
    std::size_t from = n_cities - 1;
    for (auto to : values.subspan(1)) {
        double dx = x_coords[from] - x_coords[to];
        double dy = y_coords[from] - y_coords[to];
        total_distance += std::sqrt(dx*dx + dy*dy);
        from = (from+1)%n_cities;
    }

    return total_distance;
}

bool TSP_validity_checker(std::span<const int> const values) {
    int n_cities = values[0]; // first element of the vector is the number of cities
    if (n_cities != values.size()-1)
        return false;
    bool* seen = new bool[n_cities];
    std::fill(seen, seen+n_cities, false);
    std::span<const int> cities = values.subspan(1);
    for (auto city : cities) {
        if (city < 0 || city >= n_cities || seen[city]) {
            delete[] seen;
            return false;
        }
        seen[city] = true;
    }
    delete[] seen;
    return true;

}

std::size_t get_TSP_extent(std::span<const int> const fixed_prefix) {
    // This function should return the maximim size of the vector that could be randomly generated
    // for a given fixed prefix and/or values

    // For Travelling Salesman, this is the number of cities
    // i.e 
    return fixed_prefix[0];
}

void generate_random_TSP_vector(std::span<const int> const fixed_prefix,
                             std::span<int> values,
                             RandomNumberGenerator& rng) {
                             
    std::size_t n_cities = fixed_prefix[0];
    // Need a random tour of the cities, i.e a shuffle of the numbers from 0 to n-1
    // To simplify the implementation of other functions,
    for (std::size_t i=0; i<n_cities; i++) {
        values[i] = i;
    }
    // Shuffle the vector using the Fisher-Yates algorithm
    for (std::size_t i=n_cities-1; i>0; i--) {
        std::size_t j = rng.rand(0, i);
        std::swap(values[i], values[j]);
    }
}

void mutate_TSP_vector(std::span<const int> const fixed_prefix,
                             std::span<int> values,
                             std::size_t max_mutations,
                             RandomNumberGenerator& rng) {
    // This function should mutate the vector by swapping two cities in the tour, up to max_mutations times.
    std::size_t n_cities = fixed_prefix[0];
    for (std::size_t i=0; i<max_mutations; i++) {
        std::size_t j = rng.rand(0, n_cities-1);
        std::size_t k = rng.rand(0, n_cities-1);
        std::swap(values[j], values[k]);
    }
}

void crossover_TSP_vector(std::span<const int> const fixed_prefix,
                             std::span<const int> const parent1_values,
                             std::span<const int> const parent2_values,
                             std::span<std::span<int>> child_values,
                             RandomNumberGenerator& rng) {
    

    // Order crossover is often suggested for TSP.
    std::size_t n_cities = fixed_prefix[0];

    // First take a slice from the first parent

    for (auto child : child_values) {

        for (std::size_t i=0; i<n_cities; i++) {
            child[i] = -1; // initialize child with invalid city index
        }

        std::size_t start = rng.rand(0, n_cities-1);
        std::size_t end = rng.rand(start, n_cities-1);

        for (std::size_t i=start; i<=end; i++) {
            child[i] = parent1_values[i];
        }

        // Then fill in the remaining cities from the second parent in the order they appear, skipping those already in the child
        std::size_t child_index = (end + 1) % n_cities;
        for (std::size_t i=0; i<n_cities; i++) {
            std::size_t city = parent2_values[(end + 1 + i) % n_cities];
            if (std::find(child.begin(), child.end(), city) == child.end()) {
                child[child_index] = city;
                child_index = (child_index + 1) % n_cities;
            }
        }
    }

}

double TSP_similarity(std::span<int> const fixed_prefix,
                                std::span<int> const vec1,
                                std::span<int> const vec2) {
    // This function calculates the similarity between two TSP tours
    // as the proportion of cities that are in the same position in both tours.
    std::size_t n_cities = fixed_prefix[0];
    double similarity = 0;
    for (std::size_t i=0; i<n_cities; i++) {
        similarity += (vec1[i] == vec2[i]) ? 1 : 0;
    }
    return similarity / n_cities;
}



GA_Functions ga_functions{
        .vector_extent = &get_TSP_extent,
        .vector_generator = &generate_random_TSP_vector,
        .vector_mutator = &mutate_TSP_vector,
        .vector_crossover = &crossover_TSP_vector
    };

int main() {
    // set things up

    std::cout << "Checking optimum for 6 city TSP problem" << std::endl;

    int fixed[] = {6}; // options for travelling salesman problem with 6 cities

    BaseResult<std::span<int>> result = optimize_span(std::span<int>(fixed),
                                                        &TSP_performance,
                                                        &TSP_validity_checker,
                                                        ga_functions);

    std::cout << "TSP performance: " << result.performance << std::endl;

    // You should test the quality of your answer here.

    return 0;
}