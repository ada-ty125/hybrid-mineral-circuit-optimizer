/** header file for the circuit simulator
 *
 * This header file defines the function that will be used to evaluate the circuit
 */

#pragma once

#include "CSRGraph.h"
#include "RequiredFunctions.h"
#include "CCircuit.h"

struct Simulator_Parameters {
    double tolerance = 1e-6;
    int max_iterations = 10000;
    double min_denominator = 1e-12;
};

extern Simulator_Parameters default_simulator_parameters;

class CSimulator {
  public:
    static double evaluate(
        Circuit& circuit,
        const Simulator_Parameters& simulator_parameters =
            default_simulator_parameters
    );

  private:
    static void calculate_all_outputs(Circuit& circuit);

    static void save_old_feeds(Circuit& circuit);

    static void clear_all_feeds(Circuit& circuit);

    static void add_to_unit_feed(
        Circuit& circuit,
        int unit_idx,
        const std::array<double, N_COMPONENTS>& material
    );

    static void add_to_unit_feed(
        Circuit& circuit,
        int unit_idx,
        const double material[N_COMPONENTS]
    );

    static void clear_final_outputs(Circuit& circuit);

    static void distribute_outputs(Circuit& circuit);

    static bool has_converged(
        const Circuit& circuit,
        const Simulator_Parameters& simulator_parameters = default_simulator_parameters
    );
};

