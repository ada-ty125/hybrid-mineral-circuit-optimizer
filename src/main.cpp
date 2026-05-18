#include <iostream>

#include "CSRGraph.h"
#include "RequiredFunctions.h"


std::size_t get_extent(std::span<const int> const fixed_prefix) {
    return 10; // this should be a function of the fixed prefix
}

void generate_random_vector(std::span<const int> const fixed_values,
                            std::span<int> random_values,
                            RandomNumberGenerator& rng) {
    // Generate a random vector of integers with some fixed values and some random values,
    // defined in libSimulator. This isn't required to be valid, but you can implement
    // appropriate
    for (std::size_t i=0; i<fixed_values.size(); i++) {
        random_values[i] = fixed_values[i];
    }
    for (std::size_t i=fixed_values.size(); i<random_values.size(); i++) {
        random_values[i] = rng.rand(0, fixed_values[1]+fixed_values[2]-1); // this should be a function of the fixed values
    }
}

int main(int argc, char * argv[])
{

    // set things up
    int fixed_values[] = {1, 2, 2, 2, 3};
    
    

    GA_Functions ga_functions{
        .vector_extent = &get_extent,
        .vector_generator = generate_random_vector,
    };


    BaseResult<std::span<int>> result = optimize_span(std::span<int>(fixed_values),
                                   &circuit_performance,
                                   &check_validity,
                                   ga_functions);
                                


    // or
    // optimize(11, vector, circuit_performance, Circuit::check_validity)
    // etc.

    // generate final output, save to file, etc.
    // pretty_print(result.data);
    plot_span(result.data, (char*)"example.png");


    if (result.performance >= 0) { // this condition should be based on the actual performance function and what constitutes a successful optimization
        std::cout << "Optimization successful!" << std::endl;
        return 0;
    } else {
        std::cout << "Optimization failed!" << std::endl;
    }
    return 1; 
}
