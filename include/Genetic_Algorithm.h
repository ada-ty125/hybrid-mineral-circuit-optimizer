/** Header for the Genetic Algorithm library
 *
*/

#pragma once

#include "CSRGraph.h"
#include "RequiredFunctions.h"


struct Algorithm_Parameters{
    int max_iterations;
    // other parameters for your algorithm       
};

extern Algorithm_Parameters default_ga_parameters;
// "good" default parameters for the genetic algorithm,
// which will be used if the user doesn't specify their own.

BaseResult<std::span<int>> optimize_span(
    std::span<int> const fixed_prefix,
    double (*performance_function)(const std::span<int>),
    bool (*validity_checker)(const std::span<int>),
    GA_Functions ga_functions,
    Algorithm_Parameters algorithm_parameters
);

// overloads (delete if not needed)

// Other functions and variables

bool default_validity_checker(std::span<int> const values);
