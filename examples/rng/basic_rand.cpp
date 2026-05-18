#include <cstdlib>
#include <iostream>

#include <mpi.h>

/** 
   Example program using rand from cstdlib
   to generate random numbers

   Advantages:
    - Conceptually simple to use
    - Works ok with MPI
    - Good Mersenne Twister implementation on mac/linux

   Disadvantages:
    - Not thread safe (on Windows)
    - Not very good random numbers (on Windows)
    - Makes blocking calls (on mac/linux)
    - Inconsistent behaviour across platforms

**/


int main(int argc, char* argv[]) {

    int count = 0;
    int n_trials = 50000000;

    int rank, size;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /** 
        The following block reads the seed from the command line (if provided) on the root process,
        and then scatters random seeds to all processes. This ensures that each process has a different seed,
        Could also do something like seed = time(NULL) + rank, but this is more robust and allows for reproducibility
        if a seed is provided.

        In general once things are seeded, random number generation is "easy" for MPI/distributed parallelism, since
        each process can generate its own random numbers independently. Shared memory parallelism takes more care.

    **/
    int* seed = new int[size];

    if (rank == 0 ) {
        if (argc>1)
            seed[0] = std::stoi(argv[1]);
        else
            seed[0] = time(NULL);
        srand(time(NULL));
        for (int i=1; i<size; ++i) {
            seed[i] = rand();
        }
    }
    
    MPI_Scatter(seed, size, MPI_INT, seed, size, MPI_INT, 0 , MPI_COMM_WORLD);
    srand(seed[rank]);


    int n_local_trials;
    
    if (rank == 0)
    {
        n_local_trials = n_trials/size + n_trials%size;
    }
    else
    {
        n_local_trials = n_trials/size;
    }

    // Our main loop, with OMP parallelism.
    #pragma omp parallel for reduction(+:count)
    for (int i = 0; i < n_local_trials; i++) {
        int random_number = rand() % 6 + 1;
        if (random_number % 3 == 0)
            count++;
    }

    MPI_Allreduce(MPI_IN_PLACE, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (rank == 0)
        std::cout << "Fraction of multiples of 3: " << static_cast<double>(count) / static_cast<double>(n_trials) << std::endl;

    MPI_Finalize();

    delete[] seed;

    return 0;
}