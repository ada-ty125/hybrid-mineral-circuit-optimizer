#include "omp.h"
#include <iostream>
#include <random>
#include <vector>

#include "mpi.h"

/** Example program using <random>
    to generate random numbers in a more efficient way

    Advantages:
    - Good random numbers.
    - Thread safe.
    - Works well with MPI.

    Disadvantages:
    - Slightly more complex to set up.
    - Slightly slower than cstdlib rand() (on some platforms).
    - Still room for improvement in the code.

**/


    
static std::random_device* rd(nullptr);
static std::mt19937* generator(nullptr);
#pragma omp threadprivate(generator, rd)




int main() {
    // Create a random number generator

    int rank, size;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    std::uniform_int_distribution<int> roll(1, 6);

    int count = 0;
    int n_trials = 50000000;

    int n_local_trials;
    
    if (rank == 0)
    {
        n_local_trials = n_trials/size + n_trials%size;
    }
    else
    {
        n_local_trials = n_trials/size;
    }

    // Initialize the random number generator for each thread.
    // Need to make sure this is done for all worker threads before we
    // start the parallel for region , otherwise we

    #pragma omp parallel num_threads(omp_get_max_threads())
    {
        if (rd == nullptr) {
            rd = new std::random_device();
        }

        if (generator == nullptr) {
            generator = new std::mt19937();
            generator->seed((*rd)());
        }
    }

    #pragma omp parallel for reduction(+:count)
    for (int i = 0; i<n_local_trials; i++) {

        int random_number = roll(*generator);
        if (random_number%3==0)
            count++;
    }

    MPI_Allreduce(MPI_IN_PLACE, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (rank == 0)
        std::cout << "Fraction of multiples of 3: " << count/(1.0*n_trials) << std::endl;

    MPI_Finalize();

    delete generator;
    delete rd;

    return 0;
}