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

bool check_validity(const CSRGraph& graph) {
    // This is function that checks if the graph is valid

    // Current dummy version just returns true.

    return true;
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
            return 1.0;  // Placeholder success value (replace with actual performance calculation
                         // later)
        }
    }

    // If the loop reaches max_iterations, the circuit failed to converge
    return -1e30;
}
