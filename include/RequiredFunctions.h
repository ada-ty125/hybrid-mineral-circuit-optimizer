#pragma once

#include <span>
#include <cstddef>
#include <random>

#include "GATypedefs.h"
#include "ESEGraph.h" 

using namespace ESE; // for Graph and CSRGraph types from esegraph/include

// in libGeneticAlgorithm

void seed_rngs(unsigned seed); // to seed next random number generation, defined in libGeneticAlgorithm

// Base class for your random number generator. *DO NOT* modify this abstract class,
// but you can implement a derived class with actual random number generation functionality.
// See the DefaultRandomNumberGenerator class in PRNG.h/.cpp
class RandomNumberGenerator {
protected:
static int RNG_DEFAULT_MIN;
static int RNG_MAX;
public:
    RandomNumberGenerator()=default;
    virtual ~RandomNumberGenerator()=default;
    virtual int rand(int min=RNG_DEFAULT_MIN, int max=RNG_MAX)=0;
    virtual double random(double min=0.0, double max=1.0)=0;
};


// in libGeneticAlgorithm, you should define implementations of the following functions,
// which can be used in the optimize_span function. You can also implement other helper functions as needed.

// a function which generates a random vector of integers
void default_generator(std::span<const  int> const fixed_prefix,
                             std::span<int> values,
                             RandomNumberGenerator& rng);
// a function which mutates a random vector of integers
void default_mutator(std::span<const int> const fixed_prefix,
                           std::span<int> values,
                           std::size_t max_mutations,
                           RandomNumberGenerator& rng);
// a function which crosses over two random vectors of integers to produce a child vector
void default_crossover(std::span<const int> const fixed_prefix,
                             std::span<const int> const parent1_values,
                             std::span<const int> const parent2_values,
                             std::span<std::span<int>> child_values,
                             RandomNumberGenerator& rng);
// similarity function to calculate the similarily between two vectors
double default_similarity(std::span<const int> const fixed_prefix,
                                std::span<const int> const vec1,
                                std::span<const int> const vec2);


// Struct to hold all the optional function specializations for the genetic algorithm.
// You can derive additional fields in a derived struct if you want, but must
// work ok with just these.

struct GA_Functions {
    // a function which returns the maximum extent of the random part of the vector
    std::size_t (*vector_extent)(std::span<const int> const fixed_prefix);
    // a function which generates a random vector of integers
    void (*vector_generator)(std::span<const int> const fixed_prefix,
                             std::span<int> values,
                             RandomNumberGenerator& rng) = &default_generator;
    // a function which mutates a random vector of integers
    void (*vector_mutator)(std::span<const int> const fixed_prefix,
                           std::span<int> values,
                           std::size_t max_mutations,
                           RandomNumberGenerator& rng) = &default_mutator;
    // a function which crosses over two random vectors of integers to produce a child vector
    void (*vector_crossover)(std::span<const int> const fixed_prefix,
                             std::span<const int> const parent1_values,
                             std::span<const int> const parent2_values,
                             std::span<std::span<int>> child_values,
                             RandomNumberGenerator& rng) = &default_crossover;
    // similarity function to calculate the similarily between two vectors
    double (*vector_similarity)(std::span<const int> const fixed_prefix,
                                std::span<const int> const vec1,
                                std::span<const int> const vec2) = &default_similarity;
};


template <typename T>
struct BaseResult {
    T data;
    double performance;
};

template <>
struct BaseResult<std::span<int>>{
    std::span<int> data;
    double performance;
    ~BaseResult(){
        delete[] data.data();
    }
};

BaseResult<std::span<int>> optimize_span(
    std::span<const int> const fixed_prefix,
    double (*performance_function)(const std::span<const int>),
    bool (*validity_checker)(const std::span<const int>),
    GA_Functions ga_functions
); // Optimize a random vector of integers with some fixed values and some random values, using a genetic algorithm. defined in libGeneticAlgorithm 

// optional visualization (can use Python if your team prefers)

// in libSimulator

// Any specializations of generator, mutator, crossover, similarity functions

double circuit_performance(std::span<const int> const circuit_span); // The function which calculates the overall performance
                                                                // defined in libSimulator
double circuit_performance(const Graph& graph); // Overload using ESE::Graph type from esegraph

bool check_validity(std::span<const int> const circuit_span); // A fast function which applies graph heuristics to rule out
                                                        // invalid vectors. defined in libSimulator

bool check_validity(const Graph& graph); // Overload using ESE::Graph type from esegraph

// Circuit specialized functions needed for the genetic algorithm                                                        
void generate_circuit_vector(std::span<const int> const fixed_values,
                             std::span<int> random_values,
                             RandomNumberGenerator& rng); // Generate a random vector of integers with some fixed values and some random values,
                                                         // defined in libSimulator

void mutate_circuitvector(std::span<const int> const fixed_values,
                          std::span<int> mutable_values,
                          RandomNumberGenerator& rng); // Mutate a random vector of integers with some fixed values and some random values,
                                                // defined in libSimulator

void crossover_circuit_vectors(std::span<const int> const fixed_values,
                   std::span<const int> const parent1_values,
                   std::span<const int> const parent2_values,
                   std::span<std::span<int>> child_values, // Length of outer span is the number of children, length of inner spans is the length of the random part of the vector
                   RandomNumberGenerator& rng); // Cross over two random vectors of integers with some fixed values and some random values to produce a child vector, defined in libSimulator


// Optional visualization functions, defined in libSimulator.
// Can also use Python if your team prefers.

void plot_span(std::span<const int> const values, const char* filename); // Plot a vector of integers with some fixed values and some random values, defined in libGeneticAlgorithm
void plot_graph(const Graph& graph, const char* filename);