#include <vector>
#include <array>
#include <span>
#include <stdio.h>
#include <algorithm>
#include <cmath>
#include <vector>

#include "CSRGraph.h"
#include "CUnit.h"
#include "CCircuit.h"
#include "Economics.h"

namespace {
struct ParsedCircuit {
    int num_inputs = 0;
    int num_units = 0;
    int num_products = 0;
    int feed_dest = -1;
    std::vector<std::vector<int>> outputs;
};

bool parse_circuit(std::span<const int> circuit_vector, ParsedCircuit& parsed) {
    if (circuit_vector.size() < 4) {
        return false;
    }

    parsed.num_inputs = circuit_vector[0];
    parsed.num_units = circuit_vector[1];
    parsed.num_products = circuit_vector[2];

    if (parsed.num_inputs <= 0 || parsed.num_units <= 0 || parsed.num_products < 2) {
        return false;
    }

    const std::size_t unit_count = static_cast<std::size_t>(parsed.num_units);
    const std::size_t feed_index = 3 + unit_count;
    if (circuit_vector.size() < feed_index + 1) {
        return false;
    }

    std::vector<int> output_counts(unit_count, 0);
    std::size_t total_outputs = 0;
    for (std::size_t unit = 0; unit < unit_count; ++unit) {
        const int output_count = circuit_vector[3 + unit];
        if (output_count != 2 && output_count != 3) {
            return false;
        }
        output_counts[unit] = output_count;
        total_outputs += static_cast<std::size_t>(output_count);
    }

    const std::size_t expected_length = 3 + unit_count + 1 + total_outputs;
    if (circuit_vector.size() != expected_length) {
        return false;
    }

    parsed.feed_dest = circuit_vector[feed_index];
    if (parsed.feed_dest < 0 || parsed.feed_dest >= parsed.num_units) {
        return false;
    }

    parsed.outputs.assign(unit_count, {});
    std::size_t pos = feed_index + 1;
    const int one_past_last_product_id = parsed.num_units + parsed.num_products;

    for (std::size_t unit = 0; unit < unit_count; ++unit) {
        auto& destinations = parsed.outputs[unit];
        destinations.reserve(static_cast<std::size_t>(output_counts[unit]));

        for (int out = 0; out < output_counts[unit]; ++out) {
            const int destination = circuit_vector[pos++];
            if (destination < 0 || destination >= one_past_last_product_id) {
                return false;
            }
            if (destination == static_cast<int>(unit)) {
                return false;
            }
            if (std::find(destinations.begin(), destinations.end(), destination) !=
                destinations.end()) {
                return false;
            }
            destinations.push_back(destination);
        }
    }

    return true;
}

bool all_units_reachable_from_feed(const ParsedCircuit& circuit) {
    std::vector<bool> seen(static_cast<std::size_t>(circuit.num_units), false);
    std::vector<int> stack{circuit.feed_dest};

    while (!stack.empty()) {
        const int unit = stack.back();
        stack.pop_back();

        if (unit < 0 || unit >= circuit.num_units) {
            continue;
        }

        const std::size_t unit_index = static_cast<std::size_t>(unit);
        if (seen[unit_index]) {
            continue;
        }

        seen[unit_index] = true;
        for (const int destination : circuit.outputs[unit_index]) {
            if (destination >= 0 && destination < circuit.num_units) {
                stack.push_back(destination);
            }
        }
    }

    return std::all_of(seen.begin(), seen.end(), [](bool value) { return value; });
}

int count_reachable_products(const ParsedCircuit& circuit, int start_unit) {
    std::vector<bool> seen_units(static_cast<std::size_t>(circuit.num_units), false);
    std::vector<bool> seen_products(static_cast<std::size_t>(circuit.num_products), false);
    std::vector<int> stack{start_unit};

    while (!stack.empty()) {
        const int unit = stack.back();
        stack.pop_back();

        if (unit < 0 || unit >= circuit.num_units) {
            continue;
        }

        const std::size_t unit_index = static_cast<std::size_t>(unit);
        if (seen_units[unit_index]) {
            continue;
        }

        seen_units[unit_index] = true;
        for (const int destination : circuit.outputs[unit_index]) {
            if (destination >= 0 && destination < circuit.num_units) {
                stack.push_back(destination);
            } else {
                const int product_index = destination - circuit.num_units;
                if (product_index >= 0 && product_index < circuit.num_products) {
                    seen_products[static_cast<std::size_t>(product_index)] = true;
                }
            }
        }
    }

    return static_cast<int>(std::count(seen_products.begin(), seen_products.end(), true));
}
}  // namespace

bool check_validity(std::span<const int> circuit_vector) {
    ParsedCircuit circuit;
    if (!parse_circuit(circuit_vector, circuit)) {
        return false;
    }

    if (!all_units_reachable_from_feed(circuit)) {
        return false;
    }

    for (int unit = 0; unit < circuit.num_units; ++unit) {
        if (count_reachable_products(circuit, unit) < 2) {
            return false;
        }
    }

    return true;
}

bool check_validity(const ESE::Graph& graph) {
    const int num_units = static_cast<int>(graph.n_dynamic_nodes);
    const int num_products = static_cast<int>(graph.n_sink_nodes);
    if (num_units <= 0 || num_products < 2) {
        return false;
    }

    std::vector<int> circuit_vector;
    circuit_vector.push_back(static_cast<int>(graph.n_root_nodes));
    circuit_vector.push_back(num_units);
    circuit_vector.push_back(num_products);

    try {
        for (int unit = 0; unit < num_units; ++unit) {
            circuit_vector.push_back(
                static_cast<int>(graph.get_degree(static_cast<std::size_t>(unit))));
        }

        int feed_dest = 0;
        if (graph.n_root_nodes > 0) {
            feed_dest =
                static_cast<int>(graph.node_targets(-static_cast<int>(graph.n_root_nodes), 0));
        }
        circuit_vector.push_back(feed_dest);

        for (int unit = 0; unit < num_units; ++unit) {
            const auto degree = graph.get_degree(static_cast<std::size_t>(unit));
            for (std::size_t out = 0; out < degree; ++out) {
                circuit_vector.push_back(static_cast<int>(graph.node_targets(unit, out)));
            }
        }
    } catch (...) {
        return false;
    }

    return check_validity(std::span<const int>(circuit_vector));
}

bool Circuit::check_validity(int vector_size, int* circuit_vector) {
    if (vector_size < 0 || circuit_vector == nullptr) {
        return false;
    }

    return ::check_validity(
        std::span<const int>(circuit_vector, static_cast<std::size_t>(vector_size)));
}

bool Circuit::check_validity(int vector_size, int* circuit_vector, int unit_parameters_size,
                             double* unit_parameters) {
    (void)unit_parameters_size;
    (void)unit_parameters;
    return check_validity(vector_size, circuit_vector);
}

// Constructor using circuit vector
Circuit::Circuit(std::span<const int> circuit_vector) {
    // Define the number of units based on the circuit vector
    int num_units = circuit_vector[1];

    // Define the number of products based on the circuit vector
    int num_products = circuit_vector[2];

    // Resize the units vector to hold all units
    units.resize(num_units);
    final_products.resize(num_products);
    final_tailings.fill(0.0);

    // Resize the units vector to hold all units
    units.resize(num_units);

    // Read feed destination and output destinations from the circuit vector
    // Index 0 = num_inputs
    // Index 1 = num_units
    // Index 2 = num_products
    // So unit output counts begin at index 3
    int pos = 3;

    // Read unit output counts and decide Type A / Type B
    for (int i = 0; i < num_units; i++) {
        // Set the number of outputs for the unit
        units[i].n_outputs = circuit_vector[pos];

        // Set unit constants based on the number of outputs
        // 2 outputs = Type A
        // 3 outputs = Type B
        set_unit_constants(units[i]);

        // Make space for this unit's output destination IDs
        units[i].output.resize(units[i].n_outputs);

        pos++;
        // Index 3 + num_units = start of feed destination
    }

    // Read feed destination
    feed_dest = circuit_vector[pos];
    pos++;
    // Index 3 + num_units + 1 = start of output destinations

    // Read output destinations
    for (int i = 0; i < num_units; i++) {
        for (int out = 0; out < units[i].n_outputs; out++) {
            // Set the output destination for the unit
            units[i].output[out] = circuit_vector[pos];
            pos++;
        }
    }
}

// Helper function to set unit constants based on unit type (Type A or Type B)
void Circuit::set_unit_constants(CUnit& unit) {
    if (unit.n_outputs == 2) {
        // Type A
        unit.k_matrix[0][0] = 0.008;
        unit.k_matrix[0][1] = 0.006;
        unit.k_matrix[0][2] = 0.0005;

        unit.k_matrix[1][0] = 0.0;
        unit.k_matrix[1][1] = 0.0;
        unit.k_matrix[1][2] = 0.0;
    }

    else if (unit.n_outputs == 3) {
        // Type B
        unit.k_matrix[0][0] = 0.007;
        unit.k_matrix[0][1] = 0.001;
        unit.k_matrix[0][2] = 0.001;

        unit.k_matrix[1][0] = 0.001;
        unit.k_matrix[1][1] = 0.006;
        unit.k_matrix[1][2] = 0.001;
    }
}

// Recursively marks all units reachable from unit_num
void Circuit::mark_units(int unit_num) {
    // If unit_num is not a real unit ->stop
    if (unit_num < 0 || unit_num >= static_cast<int>(units.size())) {
        return;
    }

    // If this unit has already been visited -> stop
    if (units[unit_num].mark) {
        return;
    }

    // Mark this unit as visited
    units[unit_num].mark = true;

    // Visit all units connected to this unit's outputs
    for (int i = 0; i < units[unit_num].n_outputs; i++) {
        // Destination of output i from unit_num
        int dest = units[unit_num].output[i];

        // If destination is another unit -> visit it
        // If it is a product/tailings outlet -> ignore it
        if (dest >= 0 && dest < static_cast<int>(units.size())) {
            mark_units(dest);
        }
    }
}

// Calls calculate_outputs() on every unit based on current feeds
void Circuit::calculate_all_outputs() {
    for (auto& unit : units) {
        unit.calculate_outputs();
    }
}

// Saves current feeds into old_feed to chech convergence later
void Circuit::save_old_feeds() {
    for (auto& unit : units) {
        // Save the current feed into old_feed for each component
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            unit.old_feed[comp] = unit.feed[comp];
        }
    }
}

// Clears all unit feeds before redistributing material
void Circuit::clear_all_feeds() {
    for (auto& unit : units) {
        // Clear the feed for each component
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            unit.feed[comp] = 0.0;
        }
    }
}

// Adds a concentrate stream into a unit's feed
void Circuit::add_to_unit_feed(int unit_idx, const std::array<double, N_COMPONENTS>& material) {
    // Add the material to the feed for each component
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        units[unit_idx].feed[comp] += material[comp];
    }
}

// Fix data type mismatch for tails (double[N_COMPONENTS]) vs concentrate (vector<array<double,
// N_COMPONENTS>>)

// Adds a tails stream into a unit's feed
// Needed because tails is stored as double[N_COMPONENTS]
void Circuit::add_to_unit_feed(int unit_idx, const double material[N_COMPONENTS]) {
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        units[unit_idx].feed[comp] += material[comp];
    }
}

// Helper function to clear final product and tailings outputs before redistributing new material
void Circuit::clear_final_outputs() {
    for (auto& product : final_products) {
        product.fill(0.0);
    }

    final_tailings.fill(0.0);
}

// Send every unit's outputs to the correct destination
void Circuit::distribute_outputs() {
    for (int u = 0; u < static_cast<int>(units.size()); u++) {
        // Define the current unit being processed
        CUnit& unit = units[u];

        // Number of concentrate streams based on unit typw
        // Type A: n_outputs = 2 -> num_c_streams = 1
        // Type B: n_outputs = 3 -> num_c_streams = 2
        int num_c_streams = unit.n_outputs - 1;

        // Route concentrate streams to their destinations
        for (int out = 0; out < num_c_streams; out++) {
            int dest = unit.output[out];

            if (dest >= 0 && dest < static_cast<int>(units.size())) {
                add_to_unit_feed(dest, unit.concentrate[out]);
            } else {
                int product_idx = dest - static_cast<int>(units.size());

                if (product_idx >= 0 && product_idx < static_cast<int>(final_products.size())) {
                    for (int comp = 0; comp < N_COMPONENTS; comp++) {
                        final_products[product_idx][comp] += unit.concentrate[out][comp];
                    }
                }
            }
        }

        // The last output is always tails
        int tails_dest = unit.output[unit.n_outputs - 1];

        if (tails_dest >= 0 && tails_dest < static_cast<int>(units.size())) {
            add_to_unit_feed(tails_dest, unit.tails);
        } else {
            for (int comp = 0; comp < N_COMPONENTS; comp++) {
                final_tailings[comp] += unit.tails[comp];
            }
        }
    }
}

// Checks whether the iterative simulation has converged
bool Circuit::has_converged() const {
    // Allowed relative change between old and new feeds
    constexpr double tolerance = 1e-6;

    // Small number to prevent division by zero
    constexpr double min_denominator = 1e-12;

    // Check each unit and each component for relative change in feed
    for (const auto& unit : units) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            // Feed value from previous iteration
            double old_val = unit.old_feed[comp];

            // Feed value from current iteration
            double new_val = unit.feed[comp];

            // Denominator for relative change calculation (use max to avoid division by zero)
            double denom = std::max(std::abs(old_val), min_denominator);

            // Relative change between iterations
            double rel_change = std::abs(new_val - old_val) / denom;

            // Any component in any unit changed too much -> not converged
            if (rel_change > tolerance) {
                return false;
            }
        }
    }

    return true;
}

// Main circuit simulation
double Circuit::evaluate() {
    // Set a maximum number of iterations to prevent infinite loop
    constexpr int max_iterations = 10000;

    // External feed values
    constexpr double palusznium_feed = 8.0;
    constexpr double gormanium_feed = 12.0;
    constexpr double waste_feed = 80.0;

    // Initial feed into the feed destination unit
    units[feed_dest].feed[0] = palusznium_feed;
    units[feed_dest].feed[1] = gormanium_feed;
    units[feed_dest].feed[2] = waste_feed;

    for (int iter = 0; iter < max_iterations; iter++) {
        // Each unit calculates its concentrate and tails outputs
        calculate_all_outputs();

        // Save current feeds before replacing them
        save_old_feeds();

        // Clear feeds so they can be rebuilt from routed outputs
        clear_all_feeds();

        // Add new external feeds for each component into the feed destination unit
        units[feed_dest].feed[0] += palusznium_feed;
        units[feed_dest].feed[1] += gormanium_feed;
        units[feed_dest].feed[2] += waste_feed;

        // Clear final product and tailings outputs before redistributing new material
        clear_final_outputs();

        // Send all unit outputs to their destinations
        distribute_outputs();

        // Stop if feeds are no longer changing significantly
        if (has_converged()) {
            cuprite::Stream pal_product{
                final_products[0][0],  // pal
                final_products[0][1],  // gor
                final_products[0][2]   // waste
            };

            cuprite::Stream gor_product{
                final_products[1][0],  // pal
                final_products[1][1],  // gor
                final_products[1][2]   // waste
            };

            int n_A = 0;
            int n_B = 0;
            for (const auto& unit : units) {
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

    // If the loop reaches max_iterations, fallback to your worst-case equation
    return cuprite::worst_case_value(waste_feed, cuprite::default_economics);
}
