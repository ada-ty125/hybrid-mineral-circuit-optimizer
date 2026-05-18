#include <cmath>

#include "CUnit.h"
#include "CCircuit.h"
#include "CSimulator.h"

struct Simulator_Parameters default_simulator_parameters = {0.01, 1000};

double circuit_performance(const Graph& graph){
  return circuit_performance(graph, default_simulator_parameters);
};

double circuit_performance(const  Graph& graph,
                        struct Simulator_Parameters simulator_parameters) {
// This function takes a circuit vector and returns a performance value.
// The current version of the function is just dummy function that returns
// a fixed value, but you should replace it with your actual code

  double performance = 500.0;

  return performance; 
}


double circuit_performance(std::span<const int> const vec){ return 0.0;}

bool check_validity(std::span<const int> const vector){ return true;}

bool check_validity(const Graph& graph){ return true;}

// additional overloads (delete if not needed);

// Other functions and variables to evaluate a real circuit.