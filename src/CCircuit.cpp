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
#include "CSimulator.h"

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

}  // namespace

bool check_validity(std::span<const int> circuit_vector) {
    Circuit circuit;
    if (!circuit.initialise(circuit_vector, default_simulator_parameters)) {
        return false;
    }

    const std::vector<bool> reachable_from_feed = circuit.units_reachable_from_feed();
    if (!std::all_of(reachable_from_feed.begin(), reachable_from_feed.end(),
                     [](bool value) { return value; })) {
        return false;
    }

    for (int unit = 0; unit < circuit.num_units(); ++unit) {
        if (circuit.count_reachable_products_from_unit(unit) < 2) {
            return false;
        }
    }

    return true;
}

bool check_validity(const ESE::CSRGraph& graph) {
    // -------------------------------------------------------------------------
    // STEP 1: Ultra-fast local checks from the graph representation.
    // These are the original graph-side validity checks from Validity_Checker.
    // -------------------------------------------------------------------------
    int num_units = static_cast<int>(graph.n_dynamic_nodes);
    if (num_units <= 0) {
        return false;
    }

    for (int i = 0; i < num_units; ++i) {
        int n_outputs = static_cast<int>(graph.get_degree(static_cast<std::size_t>(i)));
        if (n_outputs != 2 && n_outputs != 3) {
            return false;
        }

        int first_dest = static_cast<int>(graph.node_targets(i, 0));
        bool all_outputs_identical = true;

        for (int j = 0; j < n_outputs; ++j) {
            int dest = static_cast<int>(graph.node_targets(i, static_cast<std::size_t>(j)));

            // Condition 3: self-recycle is strictly prohibited.
            if (dest == i) {
                return false;
            }

            if (dest != first_dest) {
                all_outputs_identical = false;
            }
        }

        // Condition 4: outputs from one unit must not all merge into the same node.
        if (n_outputs > 1 && all_outputs_identical) {
            return false;
        }
    }

    // -------------------------------------------------------------------------
    // STEP 2: Build a Circuit view so the shared reachability helpers are used.
    // -------------------------------------------------------------------------
    Circuit circuit(num_units);

    int feed_destination = 0;
    if (graph.input_index != nullptr && graph.n_root_nodes > 0) {
        feed_destination = static_cast<int>(graph.input_index[0]);
    } else {
        return false;
    }

    circuit.num_inputs_ = static_cast<int>(graph.n_root_nodes);
    circuit.num_units_ = num_units;
    circuit.num_products_ = static_cast<int>(graph.n_sink_nodes);
    circuit.feed_dest_ = feed_destination;

    for (int i = 0; i < num_units; ++i) {
        CUnit& unit = circuit.units[static_cast<std::size_t>(i)];
        unit.n_outputs = static_cast<int>(graph.get_degree(static_cast<std::size_t>(i)));
        unit.unit_type = unit.n_outputs == 2 ? 0 : 1;
        unit.output.resize(static_cast<std::size_t>(unit.n_outputs));

        for (int j = 0; j < unit.n_outputs; ++j) {
            unit.output[static_cast<std::size_t>(j)] =
                static_cast<int>(graph.node_targets(i, static_cast<std::size_t>(j)));
        }
    }

    // -------------------------------------------------------------------------
    // STEP 3: Condition 1, every unit must be accessible from the feed.
    // -------------------------------------------------------------------------
    std::vector<bool> reachable_from_feed = circuit.units_reachable_from_feed();
    for (int i = 0; i < num_units; ++i) {
        if (!reachable_from_feed[static_cast<std::size_t>(i)]) {
            return false;
        }
    }

    // -------------------------------------------------------------------------
    // STEP 4: Condition 2, every unit must reach at least two product streams.
    // -------------------------------------------------------------------------
    for (int i = 0; i < num_units; ++i) {
        if (circuit.count_reachable_products_from_unit(i) < 2) {
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

Circuit::Circuit() = default;

Circuit::Circuit(int num_units) {
    if (num_units > 0) {
        num_units_ = num_units;
        units.resize(static_cast<std::size_t>(num_units));
    }
}

bool Circuit::initialise(std::span<const int> circuit_vector,
                         const Simulator_Parameters& simulator_parameters) {
    ParsedCircuit parsed;
    if (!parse_circuit(circuit_vector, parsed)) {
        return false;
    }

    const int n_comp = simulator_parameters.n_components;

    num_inputs_ = parsed.num_inputs;
    num_units_ = parsed.num_units;
    num_products_ = parsed.num_products;
    feed_dest_ = parsed.feed_dest;

    units.assign(static_cast<std::size_t>(parsed.num_units), CUnit{});

    final_products.assign(static_cast<std::size_t>(parsed.num_products),
                          std::vector<double>(static_cast<std::size_t>(n_comp), 0.0));

    final_tailings.assign(static_cast<std::size_t>(n_comp), 0.0);

    for (int unit_id = 0; unit_id < parsed.num_units; ++unit_id) {
        CUnit& unit = units[static_cast<std::size_t>(unit_id)];

        unit.n_outputs = static_cast<int>(parsed.outputs[static_cast<std::size_t>(unit_id)].size());

        unit.output = parsed.outputs[static_cast<std::size_t>(unit_id)];

        unit.feed.assign(static_cast<std::size_t>(n_comp), 0.0);
        unit.old_feed.assign(static_cast<std::size_t>(n_comp), 0.0);
        unit.tails.assign(static_cast<std::size_t>(n_comp), 0.0);
        unit.total_recovery.assign(static_cast<std::size_t>(n_comp), 0.0);

        unit.concentrate.assign(static_cast<std::size_t>(unit.n_outputs - 1),
                                std::vector<double>(static_cast<std::size_t>(n_comp), 0.0));

        unit.k_matrix.assign(2, std::vector<double>(static_cast<std::size_t>(n_comp), 0.0));

        set_unit_constants(unit, simulator_parameters);
    }

    return true;
}

bool Circuit::is_unit_id(int id) const noexcept { return id >= 0 && id < num_units(); }

bool Circuit::is_product_id(int id) const noexcept {
    return id >= num_units() && id < num_units() + num_products();
}

int Circuit::product_index(int product_id) const noexcept { return product_id - num_units(); }

std::vector<bool> Circuit::units_reachable_from_feed() const {
    std::vector<bool> seen(units.size(), false);
    if (!is_unit_id(feed_dest_)) {
        return seen;
    }

    std::vector<int> stack{feed_dest_};
    while (!stack.empty()) {
        const int unit_id = stack.back();
        stack.pop_back();

        if (!is_unit_id(unit_id)) {
            continue;
        }

        const std::size_t unit_index = static_cast<std::size_t>(unit_id);
        if (seen[unit_index]) {
            continue;
        }

        seen[unit_index] = true;
        for (const int destination : units[unit_index].output) {
            if (is_unit_id(destination)) {
                stack.push_back(destination);
            }
        }
    }

    return seen;
}

bool Circuit::can_reach(int start, int target) const {
    if (!is_unit_id(start)) {
        return false;
    }

    std::vector<bool> seen(units.size(), false);
    std::vector<int> stack{start};
    while (!stack.empty()) {
        const int unit_id = stack.back();
        stack.pop_back();

        if (!is_unit_id(unit_id)) {
            continue;
        }

        const std::size_t unit_index = static_cast<std::size_t>(unit_id);
        if (seen[unit_index]) {
            continue;
        }

        seen[unit_index] = true;
        for (const int destination : units[unit_index].output) {
            if (destination == target) {
                return true;
            }
            if (is_unit_id(destination)) {
                stack.push_back(destination);
            }
        }
    }

    return false;
}

std::vector<bool> Circuit::products_reachable_from_unit(int unit_id) const {
    std::vector<bool> product_seen(static_cast<std::size_t>(num_products()), false);
    if (!is_unit_id(unit_id)) {
        return product_seen;
    }

    std::vector<bool> seen_units(units.size(), false);
    std::vector<int> stack{unit_id};
    while (!stack.empty()) {
        const int current_unit = stack.back();
        stack.pop_back();

        if (!is_unit_id(current_unit)) {
            continue;
        }

        const std::size_t unit_index = static_cast<std::size_t>(current_unit);
        if (seen_units[unit_index]) {
            continue;
        }

        seen_units[unit_index] = true;
        for (const int destination : units[unit_index].output) {
            if (is_product_id(destination)) {
                product_seen[static_cast<std::size_t>(product_index(destination))] = true;
            } else if (is_unit_id(destination)) {
                stack.push_back(destination);
            }
        }
    }

    return product_seen;
}

int Circuit::count_reachable_products_from_unit(int unit_id) const {
    const std::vector<bool> reachable_products = products_reachable_from_unit(unit_id);
    return static_cast<int>(std::count(reachable_products.begin(), reachable_products.end(), true));
}

int Circuit::num_inputs() const noexcept { return num_inputs_; }

int Circuit::num_units() const noexcept {
    if (num_units_ > 0) {
        return num_units_;
    }
    return static_cast<int>(units.size());
}

int Circuit::num_products() const noexcept {
    if (num_products_ > 0) {
        return num_products_;
    }
    return static_cast<int>(final_products.size());
}

int Circuit::feed_dest() const noexcept { return feed_dest_; }

const std::vector<int>& Circuit::output_destinations(int unit_id) const {
    if (!is_unit_id(unit_id)) {
        return empty_outputs_;
    }
    return units[static_cast<std::size_t>(unit_id)].output;
}

// Helper function to set unit constants based on unit type (Type A or Type B)
void Circuit::set_unit_constants(CUnit& unit, const Simulator_Parameters& simulator_parameters) {
    const int n_comp = simulator_parameters.n_components;

    unit.k_matrix.assign(2, std::vector<double>(static_cast<std::size_t>(n_comp), 0.0));

    if (unit.n_outputs == 2) {
        unit.unit_type = 0;

        for (int row = 0; row < 2; ++row) {
            for (int comp = 0; comp < n_comp; ++comp) {
                if (row < static_cast<int>(simulator_parameters.k_TypeA.size()) &&
                    comp < static_cast<int>(simulator_parameters.k_TypeA[row].size())) {
                    unit.k_matrix[row][comp] = simulator_parameters.k_TypeA[row][comp];
                }
            }
        }
    } else if (unit.n_outputs == 3) {
        unit.unit_type = 1;

        for (int row = 0; row < 2; ++row) {
            for (int comp = 0; comp < n_comp; ++comp) {
                if (row < static_cast<int>(simulator_parameters.k_TypeB.size()) &&
                    comp < static_cast<int>(simulator_parameters.k_TypeB[row].size())) {
                    unit.k_matrix[row][comp] = simulator_parameters.k_TypeB[row][comp];
                }
            }
        }
    }
}

void Circuit::mark_units(int unit_num) {
    if (!is_unit_id(unit_num)) {
        return;
    }

    if (units[static_cast<std::size_t>(unit_num)].mark) {
        return;
    }

    units[static_cast<std::size_t>(unit_num)].mark = true;

    for (int destination : units[static_cast<std::size_t>(unit_num)].output) {
        if (is_unit_id(destination)) {
            mark_units(destination);
        }
    }
}
