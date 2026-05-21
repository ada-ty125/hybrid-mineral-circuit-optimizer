/**
 * @file CSimulator.cpp
 * @brief Implementation of the circuit simulator and the associated performance 
 * evaluation function.
 *
 * This file contains the implementation of the CSimulator class, which provides
 * the functionality to evaluate the performance of a given circuit configuration.
 * The main function of interest is `evaluate`, which simulates the circuit and
 * calculates a performance score based on the final product and tailings outputs.
 */

#include "RequiredFunctions.h"
#include "CCircuit.h"
#include "CSRGraph.h"
#include "Economics.h"
#include "CSimulator.h"

#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <span>


Simulator_Parameters default_simulator_parameters;

/**
 * @brief Calculates the outputs of all units in the circuit based on their 
 * current feeds and parameters.
 *
 * This function iterates through each unit in the circuit and calls its
 * `calculate_outputs` method, which computes the concentrate and tailings outputs
 *
 * @param circuit The circuit to simulate.
 * @param params The simulator parameters.
 */
void CSimulator::calculate_all_outputs(Circuit& circuit, const Simulator_Parameters& params) {
    for (auto& unit : circuit.units) {
        unit.calculate_outputs(params);
    }
}

/**
 * @brief Saves the current feeds of all units in the circuit to their `old_feed` arrays.
 *
 * This function is used to store the previous iteration's feed values before they are updated
 * in the next iteration of the simulation. This allows for convergence checking.
 *
 * @param circuit The circuit to simulate.
 */
void CSimulator::save_old_feeds(Circuit& circuit) {

    // Loop though each unit and copy the current feed values to the old_feed arrays
    for (auto& unit : circuit.units) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            unit.old_feed[comp] = unit.feed[comp];
        }

        unit.old_feed_P = unit.old_feed[0];
        unit.old_feed_G = unit.old_feed[1];
        unit.old_feed_W = unit.old_feed[2];
    }
}

/**
 * @brief Clears the feed values of all units in the circuit by setting them to zero.
 *
 * This function iterates through each unit in the circuit and sets all its feed
 * values to zero. This allows the simulation to reset the state of the circuit.
 *
 * @param circuit The circuit to simulate.
 */
void CSimulator::clear_all_feeds(Circuit& circuit) {
    for (auto& unit : circuit.units) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            unit.feed[comp] = 0.0;
        }

        unit.feed_P = 0.0;
        unit.feed_G = 0.0;
        unit.feed_W = 0.0;
    }
}

/**
 * @brief Adds the specified material to the feed of a given unit in the circuit.
 *
 * This function takes a unit index and an array of material components, and adds 
 * these components to the corresponding feed values of the specified unit.
 *
 * @param circuit The circuit to simulate.
 * @param unit_idx The index of the unit to which the material should be added.
 * @param material An array containing the amounts of each component to add to the unit's feed
 */
void CSimulator::add_to_unit_feed(Circuit& circuit, int unit_idx,
                                  const std::array<double, N_COMPONENTS>& material) {

    // Loop through each component and add the corresponding material to the unit's feed                                
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        circuit.units[unit_idx].feed[comp] += material[comp];
    }

    circuit.units[unit_idx].feed_P = circuit.units[unit_idx].feed[0];
    circuit.units[unit_idx].feed_G = circuit.units[unit_idx].feed[1];
    circuit.units[unit_idx].feed_W = circuit.units[unit_idx].feed[2];
}

/**
 * @brief Adds the specified material to the feed of a given unit in the circuit.
 *
 * This function takes a unit index and an array of material components, and adds 
 * these components to the corresponding feed values of the specified unit.
 *
 * @param circuit The circuit to simulate.
 * @param params The simulator parameters.
 */
void CSimulator::add_to_unit_feed(Circuit& circuit, int unit_idx,
                                  const double material[N_COMPONENTS]) {
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        circuit.units[unit_idx].feed[comp] += material[comp];
    }

    circuit.units[unit_idx].feed_P = circuit.units[unit_idx].feed[0];
    circuit.units[unit_idx].feed_G = circuit.units[unit_idx].feed[1];
    circuit.units[unit_idx].feed_W = circuit.units[unit_idx].feed[2];
}

/**
 * @brief Clears the final product and tailings outputs of the circuit by setting them to zero.
 *
 * This function iterates through each final product in the circuit and sets all its values to zero.
 * It also sets the tailings output values to zero.
 *
 * @param circuit The circuit to simulate.
 */
void CSimulator::clear_final_outputs(Circuit& circuit) {
    for (auto& product : circuit.final_products) {
        product.fill(0.0);
    }

    circuit.final_tailings.fill(0.0);
}

/**
 * @brief Distributes the outputs from each unit in the circuit to their respective destinations.
 *
 * This function iterates through each unit in the circuit and distributes its outputs
 * to the appropriate destinations based on the unit's output connections.
 *
 * @param circuit The circuit to simulate.
 * @param params The simulator parameters.
 */
void CSimulator::distribute_outputs(Circuit& circuit) {
    auto route_material = [&circuit](int dest, const auto& material) {

        // Check if the destination is a valid unit index
        if (dest >= 0 && dest < static_cast<int>(circuit.units.size())) {
            CSimulator::add_to_unit_feed(circuit, dest, material);
            return;
        }

        // If the destination is not a unit, check if it's a valid product index
        const int product_idx = dest - static_cast<int>(circuit.units.size());

        if (product_idx < 0 || product_idx >= circuit.num_products()) {
            return;
        }

        // If the destination is the last product, route to tailings
        if (product_idx == circuit.num_products() - 1) {
            for (int comp = 0; comp < N_COMPONENTS; comp++) {
                circuit.final_tailings[comp] += material[comp];
            }
            return;
        }

        // Otherwise, route to the appropriate final product
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            circuit.final_products[static_cast<std::size_t>(product_idx)][comp] += material[comp];
        }
    };

    // Loop through each unit and route its outputs to the appropriate destinations
    for (int u = 0; u < static_cast<int>(circuit.units.size()); u++) {
        CUnit& unit = circuit.units[static_cast<std::size_t>(u)];

        // The number of concentrate streams is one less than the total number of outputs
        int num_c_streams = unit.n_outputs - 1;

        // Route each concentrate stream to its destination
        for (int out = 0; out < num_c_streams; out++) {
            int dest = unit.output[static_cast<std::size_t>(out)];
            route_material(dest, unit.concentrate[static_cast<std::size_t>(out)]);
        }

        // Route the tailings stream to its destination
        int tails_dest = unit.output[static_cast<std::size_t>(unit.n_outputs - 1)];
        route_material(tails_dest, unit.tails);
    }
}

/**
 * @brief Checks if the simulation has converged by comparing the current feed values
 * of each unit to their previous feed values.
 *
 * This function iterates through each unit in the circuit and calculates the relative change 
 * in feed values compared to the previous iteration. If any unit has a relative change 
 * greater than the specified tolerance in the simulator parameters, the function returns false,
 * indicating that the simulation has not yet converged.
 *
 * @param circuit The circuit to simulate.
 * @param params The simulator parameters.
 * @return true if the simulation has converged, false otherwise.
 */
bool CSimulator::has_converged(const Circuit& circuit,
                               const Simulator_Parameters& simulator_parameters) {
    
    // Loops through each unit and component
    for (const auto& unit : circuit.units) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {

            // Calculate the relative change in feed values for this component
            double old_val = unit.old_feed[comp];
            double new_val = unit.feed[comp];

            // Use the maximum of the absolute old value and a small minimum denominator to avoid 
            // division by zero
            double denom = std::max(std::abs(old_val), simulator_parameters.min_denominator);
            double rel_change = std::abs(new_val - old_val) / denom;

            if (rel_change > simulator_parameters.tolerance) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Evaluates the performance of a given circuit configuration by simulating its operation and
 * calculating a performance score based on the final product and tailings outputs.
 *
 * This function initializes the feed values for the circuit, then iteratively calculates the outputs,
 * updates the feeds, and checks for convergence. If the simulation converges within the maximum number
 * of iterations, it calculates the economic value of the final products and returns it as the performance
 * score.
 *
 * @param circuit The circuit to simulate.
 * @param params The simulator parameters.
 */
double CSimulator::evaluate(Circuit& circuit, const Simulator_Parameters& simulator_parameters) {
    if (circuit.feed_dest_ < 0 || circuit.feed_dest_ >= static_cast<int>(circuit.units.size())) {
        return cuprite::worst_case_value(simulator_parameters.waste_feed,
                                         cuprite::default_economics);
    }

    if (circuit.num_products() < 2 || circuit.final_products.size() < 2) {
        return cuprite::worst_case_value(simulator_parameters.waste_feed,
                                         cuprite::default_economics);
    }

    circuit.units[circuit.feed_dest_].feed[0] = simulator_parameters.palusznium_feed;
    circuit.units[circuit.feed_dest_].feed[1] = simulator_parameters.gormanium_feed;
    circuit.units[circuit.feed_dest_].feed[2] = simulator_parameters.waste_feed;

    circuit.units[circuit.feed_dest_].feed_P = circuit.units[circuit.feed_dest_].feed[0];
    circuit.units[circuit.feed_dest_].feed_G = circuit.units[circuit.feed_dest_].feed[1];
    circuit.units[circuit.feed_dest_].feed_W = circuit.units[circuit.feed_dest_].feed[2];

    for (int iter = 0; iter < simulator_parameters.max_iterations; iter++) {
        calculate_all_outputs(circuit, simulator_parameters);
        save_old_feeds(circuit);
        clear_all_feeds(circuit);

        circuit.units[circuit.feed_dest_].feed[0] += simulator_parameters.palusznium_feed;
        circuit.units[circuit.feed_dest_].feed[1] += simulator_parameters.gormanium_feed;
        circuit.units[circuit.feed_dest_].feed[2] += simulator_parameters.waste_feed;

        circuit.units[circuit.feed_dest_].feed_P = circuit.units[circuit.feed_dest_].feed[0];
        circuit.units[circuit.feed_dest_].feed_G = circuit.units[circuit.feed_dest_].feed[1];
        circuit.units[circuit.feed_dest_].feed_W = circuit.units[circuit.feed_dest_].feed[2];

        clear_final_outputs(circuit);
        distribute_outputs(circuit);

        if (has_converged(circuit, simulator_parameters)) {
            cuprite::Stream pal_product{circuit.final_products[0][0], circuit.final_products[0][1],
                                        circuit.final_products[0][2]};

            cuprite::Stream gor_product{circuit.final_products[1][0], circuit.final_products[1][1],
                                        circuit.final_products[1][2]};

            int n_A = 0;
            int n_B = 0;

            for (const auto& unit : circuit.units) {
                if (unit.n_outputs == 2) {
                    n_A++;
                } else if (unit.n_outputs == 3) {
                    n_B++;
                }
            }

            cuprite::CircuitDescriptor descriptor{n_A, n_B};

            return cuprite::economic_value(pal_product, gor_product, descriptor,
                                           cuprite::fixed_op_cost, cuprite::default_economics);
        }
    }

    return cuprite::worst_case_value(simulator_parameters.waste_feed, cuprite::default_economics);
}

/**
 * @brief Calculates the circuit performance for a given circuit based on its graphical 
 * representation as a CSRGraph.
 *
 * This function takes a CSRGraph representation of the circuit and simulator parameters, 
 * extracts the necessary information to construct a Circuit object, and then evaluates the 
 * performance of that circuit.
 *
 * @param graph The CSRGraph representation of the circuit to evaluate.
 * @param params The simulator parameters.
 * @return The performance score of the circuit based on the economic value of its outputs.
 */
double circuit_performance(const ESE::Graph& graph, Simulator_Parameters simulator_parameters) {
    const ESE::CSRGraph& csr_graph = static_cast<const ESE::CSRGraph&>(graph);

    // Explicit guard clause check to filter dead graphs before evaluating
    if (!check_validity(csr_graph)) {
        return cuprite::worst_case_value(simulator_parameters.waste_feed,
                                         cuprite::default_economics);
    }

    int num_units = static_cast<int>(csr_graph.n_dynamic_nodes);

    std::vector<int> circuit_vec;
    circuit_vec.push_back(1);
    circuit_vec.push_back(num_units);
    circuit_vec.push_back(static_cast<int>(csr_graph.n_sink_nodes));

    for (int i = 0; i < num_units; i++) {
        int out_edges = csr_graph.output_index[i + 1] - csr_graph.output_index[i];
        circuit_vec.push_back(out_edges);
    }

    int true_feed_destination = 0;
    if (csr_graph.input_index != nullptr && csr_graph.n_root_nodes > 0) {
        true_feed_destination = static_cast<int>(csr_graph.input_index[0]);
    }  // explicit path lookup
    circuit_vec.push_back(true_feed_destination);

    for (int i = 0; i < num_units; i++) {
        int out_edges = csr_graph.output_index[i + 1] - csr_graph.output_index[i];

        for (int j = 0; j < out_edges; j++) {
            int edge_idx = csr_graph.output_index[i] + j;
            circuit_vec.push_back(csr_graph.input_index[edge_idx]);
        }
    }

    Circuit circuit;

    if (!circuit.initialise(circuit_vec, simulator_parameters)) {
        return cuprite::worst_case_value(simulator_parameters.waste_feed,
                                         cuprite::default_economics);
    }

    return CSimulator::evaluate(circuit, simulator_parameters);
}

/**
 * @brief Calculates the circuit performance for a given circuit based on its vector 
 * representation as a span of integers.
 *
 * This function takes a span of integers representing the circuit configuration and simulator 
 * parameters, constructs a Circuit object, and then evaluates the performance of that circuit. 
 *
 * @param circuit_span The vector representation of the circuit to evaluate.
 * @return The performance score of the circuit based on the economic value of its outputs.
 */
double circuit_performance(std::span<const int> const circuit_span) {
    if (!check_validity(circuit_span)) {
        return cuprite::worst_case_value(default_simulator_parameters.waste_feed,
                                         cuprite::default_economics);
    }

    Circuit circuit;

    if (!circuit.initialise(circuit_span, default_simulator_parameters)) {
        return cuprite::worst_case_value(default_simulator_parameters.waste_feed,
                                         cuprite::default_economics);
    }

    return CSimulator::evaluate(circuit, default_simulator_parameters);
}