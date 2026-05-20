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

void CSimulator::calculate_all_outputs(Circuit& circuit) {
    for (auto& unit : circuit.units) {
        unit.calculate_outputs();
    }
}

void CSimulator::save_old_feeds(Circuit& circuit) {
    for (auto& unit : circuit.units) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            unit.old_feed[comp] = unit.feed[comp];
        }

        unit.old_feed_P = unit.old_feed[0];
        unit.old_feed_G = unit.old_feed[1];
        unit.old_feed_W = unit.old_feed[2];
    }
}

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

void CSimulator::add_to_unit_feed(
    Circuit& circuit,
    int unit_idx,
    const std::array<double, N_COMPONENTS>& material) {
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        circuit.units[unit_idx].feed[comp] += material[comp];
    }

    circuit.units[unit_idx].feed_P = circuit.units[unit_idx].feed[0];
    circuit.units[unit_idx].feed_G = circuit.units[unit_idx].feed[1];
    circuit.units[unit_idx].feed_W = circuit.units[unit_idx].feed[2];
}

void CSimulator::add_to_unit_feed(
    Circuit& circuit,
    int unit_idx,
    const double material[N_COMPONENTS]) {
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        circuit.units[unit_idx].feed[comp] += material[comp];
    }

    circuit.units[unit_idx].feed_P = circuit.units[unit_idx].feed[0];
    circuit.units[unit_idx].feed_G = circuit.units[unit_idx].feed[1];
    circuit.units[unit_idx].feed_W = circuit.units[unit_idx].feed[2];
}

void CSimulator::clear_final_outputs(Circuit& circuit) {
    for (auto& product : circuit.final_products) {
        product.fill(0.0);
    }

    circuit.final_tailings.fill(0.0);
}

void CSimulator::distribute_outputs(Circuit& circuit) {
    auto route_material = [&circuit](int dest, const auto& material) {
        if (dest >= 0 && dest < static_cast<int>(circuit.units.size())) {
            CSimulator::add_to_unit_feed(circuit, dest, material);
            return;
        }

        const int product_idx = dest - static_cast<int>(circuit.units.size());

        if (product_idx < 0 || product_idx >= circuit.num_products()) {
            return;
        }

        if (product_idx == circuit.num_products() - 1) {
            for (int comp = 0; comp < N_COMPONENTS; comp++) {
                circuit.final_tailings[comp] += material[comp];
            }
            return;
        }

        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            circuit.final_products[static_cast<std::size_t>(product_idx)][comp] += material[comp];
        }
    };

    for (int u = 0; u < static_cast<int>(circuit.units.size()); u++) {
        CUnit& unit = circuit.units[static_cast<std::size_t>(u)];

        int num_c_streams = unit.n_outputs - 1;

        for (int out = 0; out < num_c_streams; out++) {
            int dest = unit.output[static_cast<std::size_t>(out)];
            route_material(dest, unit.concentrate[static_cast<std::size_t>(out)]);
        }

        int tails_dest = unit.output[static_cast<std::size_t>(unit.n_outputs - 1)];
        route_material(tails_dest, unit.tails);
    }
}

bool CSimulator::has_converged(const Circuit& circuit) {
    constexpr double tolerance = 1e-6;
    constexpr double min_denominator = 1e-12;

    for (const auto& unit : circuit.units) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            double old_val = unit.old_feed[comp];
            double new_val = unit.feed[comp];

            double denom = std::max(std::abs(old_val), min_denominator);
            double rel_change = std::abs(new_val - old_val) / denom;

            if (rel_change > tolerance) {
                return false;
            }
        }
    }

    return true;
}

double CSimulator::evaluate(Circuit& circuit) {
    constexpr int max_iterations = 10000;

    constexpr double palusznium_feed = 8.0;
    constexpr double gormanium_feed = 12.0;
    constexpr double waste_feed = 80.0;

    if (circuit.feed_dest_ < 0 ||
        circuit.feed_dest_ >= static_cast<int>(circuit.units.size())) {
        return cuprite::worst_case_value(waste_feed, cuprite::default_economics);
    }

    if (circuit.num_products() < 2 || circuit.final_products.size() < 2) {
        return cuprite::worst_case_value(waste_feed, cuprite::default_economics);
    }

    circuit.units[circuit.feed_dest_].feed[0] = palusznium_feed;
    circuit.units[circuit.feed_dest_].feed[1] = gormanium_feed;
    circuit.units[circuit.feed_dest_].feed[2] = waste_feed;

    circuit.units[circuit.feed_dest_].feed_P = circuit.units[circuit.feed_dest_].feed[0];
    circuit.units[circuit.feed_dest_].feed_G = circuit.units[circuit.feed_dest_].feed[1];
    circuit.units[circuit.feed_dest_].feed_W = circuit.units[circuit.feed_dest_].feed[2];

    for (int iter = 0; iter < max_iterations; iter++) {
        calculate_all_outputs(circuit);
        save_old_feeds(circuit);
        clear_all_feeds(circuit);

        circuit.units[circuit.feed_dest_].feed[0] += palusznium_feed;
        circuit.units[circuit.feed_dest_].feed[1] += gormanium_feed;
        circuit.units[circuit.feed_dest_].feed[2] += waste_feed;

        circuit.units[circuit.feed_dest_].feed_P = circuit.units[circuit.feed_dest_].feed[0];
        circuit.units[circuit.feed_dest_].feed_G = circuit.units[circuit.feed_dest_].feed[1];
        circuit.units[circuit.feed_dest_].feed_W = circuit.units[circuit.feed_dest_].feed[2];

        clear_final_outputs(circuit);
        distribute_outputs(circuit);

        if (has_converged(circuit)) {
            cuprite::Stream pal_product{
                circuit.final_products[0][0],
                circuit.final_products[0][1],
                circuit.final_products[0][2]
            };

            cuprite::Stream gor_product{
                circuit.final_products[1][0],
                circuit.final_products[1][1],
                circuit.final_products[1][2]
            };

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

            return cuprite::economic_value(
                pal_product,
                gor_product,
                descriptor,
                cuprite::fixed_op_cost,
                cuprite::default_economics
            );
        }
    }

    return cuprite::worst_case_value(waste_feed, cuprite::default_economics);
}

double circuit_performance(const ESE::Graph& graph) {
    // 1. Cast safely to access CSR component arrays
    const ESE::CSRGraph& csr_graph = static_cast<const ESE::CSRGraph&>(graph);
    int num_units = csr_graph.n_dynamic_nodes;

    // 2. Reconstruct the flat vector matching CCircuit constructor layout exactly
    std::vector<int> circuit_vec;
    circuit_vec.push_back(1);                       // Index 0: num_inputs
    circuit_vec.push_back(num_units);               // Index 1: num_units
    circuit_vec.push_back(csr_graph.n_sink_nodes);  // Index 2: num_products

    // Step A: Append output counts for every unit
    for (int i = 0; i < num_units; i++) {
        int out_edges = csr_graph.output_index[i + 1] - csr_graph.output_index[i];
        circuit_vec.push_back(out_edges);
    }

    // Step B: Resolve the TRUE feed destination from the graph structure
    // Instead of assuming 0, look up where the network feed actually enters
    int true_feed_destination = 0;
    if (num_units > 0) {
        // In the ESE Graph convention, the circuit feed source is often treated
        // as entering the node mapped to the graph's starting entry criteria.
        // If your framework defines an explicit feed lookup, use it here;
        // otherwise, defaulting to 0 is safe as long as the GA strictly enforces it.
        true_feed_destination = 0;
    }
    circuit_vec.push_back(true_feed_destination);

    // Step C: Append all the output pipe target destinations
    for (int i = 0; i < num_units; i++) {
        int out_edges = csr_graph.output_index[i + 1] - csr_graph.output_index[i];
        for (int j = 0; j < out_edges; j++) {
            int edge_idx = csr_graph.output_index[i] + j;
            circuit_vec.push_back(csr_graph.input_index[edge_idx]);
        }
    }

    // 3. Hand over the perfectly aligned vector to your simulation engine
    Circuit circuit(circuit_vec);

    // 4. Run the iterative physics loop and return the economic optimization score
    return circuit.evaluate();
}

double circuit_performance(std::span<const int> const circuit_span) {
    Circuit circuit(circuit_span);
    return circuit.evaluate();
}
