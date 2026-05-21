/** header file for the circuit simulator
 *
 * This header file defines the function that will be used to evaluate the circuit
 */

#pragma once

#include "CSRGraph.h"
#include "RequiredFunctions.h"
#include "CCircuit.h"
#include <span>
#include <vector>

class CSimulator {
  public:
    static double evaluate(Circuit& circuit, const Simulator_Parameters& simulator_parameters =
                                                 default_simulator_parameters);

  private:
    static void calculate_all_outputs(Circuit& circuit,
                                      const Simulator_Parameters& simulator_parameters);

    static void save_old_feeds(Circuit& circuit);

    static void clear_all_feeds(Circuit& circuit);

    static void add_to_unit_feed(Circuit& circuit, int unit_idx,
                                 const std::vector<double>& material);

    static void add_to_unit_feed(Circuit& circuit, int unit_idx,
                                 const std::vector<double>& material);

    static void clear_final_outputs(Circuit& circuit);

    static void distribute_outputs(Circuit& circuit);

    static bool has_converged(
        const Circuit& circuit,
        const Simulator_Parameters& simulator_parameters = default_simulator_parameters);
};
double circuit_performance(const ESE::Graph& graph, Simulator_Parameters simulator_parameters =
                                                        default_simulator_parameters);

// The original standard span fallback version
double circuit_performance(std::span<const int> const circuit_span);