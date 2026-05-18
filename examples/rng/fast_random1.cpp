#include "omp.h"
#include <iostream>
#include <random>
#include <vector>

#include "mpi.h"

/** 
    Example program using <random>
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


// this is a global variable, we could
// wrap in a class or use a namespace etc.
static std::mt19937 *generators;

int initialise_generators() {
    int num_threads = omp_get_max_threads();

    // allocate memory for the generators
    generators = new std::mt19937[num_threads];

    // seed each generator with a different seed
    std::random_device rd;
    for (int i = 0; i < num_threads; i++) {
        generators[i].seed(rd());
    }

    return num_threads;
}

int finalise_generators() {
    // free memory
    delete[] generators;
    return 0;
}

int main() {
    // Create a random number generator

    int rank, size;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    initialise_generators();

    std::uniform_int_distribution<int> roll(1, 6);

    int thread_id = -1;
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

    

    #pragma omp parallel for firstprivate(thread_id) reduction(+:count)
    for (int i = 0; i<n_local_trials; i++) {
        // omp_get_thread_num isn't free, so
        // only call it once per thread
        if (thread_id == -1) 
          thread_id = omp_get_thread_num();
        int random_number = roll(generators[thread_id]);
        if (random_number%3==0)
            count++;
    }

    MPI_Allreduce(MPI_IN_PLACE, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (rank == 0)
        std::cout << "Fraction of multiples of 3: " << count/(1.0*n_trials) << std::endl;

    finalise_generators();

    MPI_Finalize();

    return 0;
}