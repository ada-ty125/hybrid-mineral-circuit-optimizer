#include <stdio.h>
#include <cstdlib> // for rand and srand (you probably want to use a better RNG)
#include <time.h>
#include <random>

#include "CSRGraph.h"
#include "PRNG.h"
#include "Genetic_Algorithm.h"

Algorithm_Parameters default_ga_parameters{1000};

BaseResult<std::span<int>> optimize_span(
    std::span<const int> const fixed_prefix,
    double (*performance_function)(const std::span<const int>),
    bool (*validity_checker)(const std::span<const int>),
    GA_Functions ga_functions,
    Algorithm_Parameters algorithm_parameters
){
/*
     This function optimizes a given set of node degrees of graph for a given performance function using a genetic algorithm.
     The function takes as input the sizes of the graph, a performance function that evaluates the performance of a
     given graph, a graph generator function that generates random graphs of the given size,
     and a validity checker function that checks if a given graph is valid (i.e. provides a short-cut to performance measurment)
     
     The function should return the best graph found by the genetic algorithm, 
     or a graph with a negative node count if it fails to find an optimal solution.
      
     The precise value return can have meaning if you want it to.

  */

  // Run the genetic algorithm process

    
  std::size_t extent = ga_functions.vector_extent(fixed_prefix);

  int* working_data = new int[extent];
  DefaultRandomNumberGenerator rng; // Seeded with current time

  ga_functions.vector_generator(fixed_prefix,
                                std::span<int>(working_data, extent),
                                rng);

  int* best_solution = new int[fixed_prefix.size()+extent];

  std::copy(fixed_prefix.begin(), fixed_prefix.end(), best_solution);


  // Update the vector with the best solution found and output the result in some way.

  delete[] working_data;
  
  // This is just a placeholder return value. You should replace it with your real one.
  return BaseResult<std::span<int>>{std::span<int>(best_solution, fixed_prefix.size()+extent), 0.0} ;

}

BaseResult<std::span<int>> optimize_span(
    std::span<const int> const fixed_prefix,
    double (*performance_function)(const std::span<const int>),
    bool (*validity_checker)(const std::span<const int>),
    GA_Functions ga_functions
){

return optimize_span(fixed_prefix, performance_function,
                      validity_checker, ga_functions, default_ga_parameters);
};

bool default_validity_checker(std::span<const int> const values) {
    // This validity checker always returns true
    // Use to skip validity checks
    return true;
}

void default_generator(std::span<const int> const fixed_prefix,
                             std::span<int> values,
                             RandomNumberGenerator& rng) {
    // This is a default generator which generates a random vector of integers with some fixed values and some random values, where the random values are between 0 and 10, while keeping the fixed prefix unchanged.

}

void default_mutator(std::span<const int> const fixed_prefix,
               std::span<int> values,
               std::size_t max_mutations,
               RandomNumberGenerator& rng) {
    // This is a default mutation function which randomly mutates up to max_mutations entries in the vector, while keeping the fixed prefix unchanged.
}

void default_crossover(std::span<const int> const fixed_prefix,
                             std::span<const int> const parent1_values,
                             std::span<const int> const parent2_values,
                             std::span<std::span<int>> child_values,
                             RandomNumberGenerator& rng) {
                              for (auto child : child_values) {
                                  // do things to implement crossover
                              }

                             }

double default_similarity(std::span<const int> const fixed_prefix,
                                std::span<const int> const vec1,
                                std::span<const int> const vec2) {
     // Default similarity function
     double similarity = 0;
      for (std::size_t i=0; i<vec1.size(); i++) {
          similarity += (vec1[i] == vec2[i]) ? 1 : 0;
      }
      return similarity / vec1.size();
    }

// overloads (delete if not needed)


// additional variables, classes and functions as needed.