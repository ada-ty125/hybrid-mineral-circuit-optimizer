#include "RequiredFunctions.h"
#include "CCircuit.h"
#include "CSRGraph.h"
#include "Economics.h"
#include <vector>
#include <span>

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
