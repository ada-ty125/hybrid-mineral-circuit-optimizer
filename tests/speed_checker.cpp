#include <iostream>
#include <chrono>
#include <string>

#include "CSRGraph.h"
#include "RequiredFunctions.h"


int main(int argc, char* argv[]) {
    int num_iterations = 1000; // Default number of iterations
    if (argc > 1) {
        num_iterations = std::stoi(argv[1]);
    }

    std::vector<CSRGraph> test_graphs = CSRGraph::read_graphs_from_file("data/test_graphs.txt");

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int ctr=0; ctr<num_iterations; ctr++) {
        double performance = circuit_performance(test_graphs[ctr % test_graphs.size()]);
    }
    

    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_seconds = end_time - start_time;
    std::cout << "Elapsed time for " << num_iterations << " iterations: " << elapsed_seconds.count() << " seconds" << std::endl;



    return 0;
}