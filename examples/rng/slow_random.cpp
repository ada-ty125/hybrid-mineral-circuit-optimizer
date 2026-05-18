#include <iostream>
#include <random>

#include "mpi.h"

/* Example program misusing <random>
   to generate random numbers

   Advantages:
    - Good random numbers
    - Thread safe
    - Works ok (but slowly) with MPI

   Disadvantages:
    - Very slow due to reseeding/recreating the generator
      inside the main loop

*/

int main() {

    int rank, size;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

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

    #pragma omp parallel for
    for (int i = 0; i<n_local_trials; i++) {
        std::random_device rd; // slow thing in main loop
        std::mt19937 gen(rd());// slow thing in main loop
        std::uniform_int_distribution<int> roll(1, 6);
        int random_number = roll(gen);
        if (random_number%3==0)
            count++;
    }

    MPI_Allreduce(MPI_IN_PLACE, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (rank == 0)
        std::cout << "Fraction of multiples of 3: " << count/(1.0*n_trials) << std::endl;

    MPI_Finalize();

    return 0;
}