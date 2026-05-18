#include <cstdio>
#include <omp.h>

int main() {

    printf("Running on %d threads.\n", omp_get_max_threads());

#pragma omp parallel for
    for (int i = 0; i < 20; ++i) {
        int tid = omp_get_thread_num();
        int num_threads = omp_get_num_threads();
        printf("%d. Thread %d of %d says hello!\n", i, tid, num_threads);
    }
}