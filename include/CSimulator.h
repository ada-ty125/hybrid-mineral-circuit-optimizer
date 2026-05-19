/** header file for the circuit simulator
 *
 * This header file defines the function that will be used to evaluate the circuit
 */

#pragma once

#include "CSRGraph.h"
#include "RequiredFunctions.h"

struct Simulator_Parameters {
    double tolerance;
    int max_iterations;
    // other parameters for your circuit simulator
};

extern Simulator_Parameters default_simulator_parameters;

double circuit_performance(const ESE::Graph& graph,
                           struct Simulator_Parameters simulator_parameters);
