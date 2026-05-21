/**
 * @file post_processing_test.cpp
 * @brief Verifies DOT post-processing output for circuit vectors and graphs.
 *
 * The test writes small visualizations and checks that expected nodes and
 * edges are present in the generated DOT text.
 */

#include <fstream>
#include <iostream>
#include <span>
#include <sstream>
#include <string>

#include "CSRGraph.h"
#include "RequiredFunctions.h"

namespace {

std::string read_file(const char* filename) {
    std::ifstream input(filename);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool contains(const std::string& text, const std::string& expected) {
    return text.find(expected) != std::string::npos;
}

bool require_contains(const std::string& text, const std::string& expected) {
    if (contains(text, expected)) {
        return true;
    }
    std::cerr << "Expected output to contain: " << expected << "\n";
    return false;
}

}  // namespace

int main() {
    bool ok = true;

    int circuit[] = {1, 3, 3, 2, 3, 2, 0, 1, 2, 3, 4, 5, 4, 5};
    plot_span(std::span<const int>(circuit), "post_processing_circuit.dot");
    const std::string circuit_dot = read_file("post_processing_circuit.dot");

    ok &= require_contains(circuit_dot, "digraph circuit");
    ok &= require_contains(circuit_dot, "feed -> unit_0");
    ok &= require_contains(circuit_dot, "unit_0 -> unit_1");
    ok &= require_contains(circuit_dot, "Unit 1\\nType B");
    ok &= require_contains(circuit_dot, "unit_1 -> palusznium_product");
    ok &= require_contains(circuit_dot, "unit_1 -> gormanium_product");
    ok &= require_contains(circuit_dot, "unit_2 -> tailings_product");

    if (contains(circuit_dot, "Error:")) {
        std::cerr << "Valid circuit unexpectedly produced an error DOT graph\n";
        ok = false;
    }

    int malformed[] = {1, 2, 3, 2};
    plot_span(std::span<const int>(malformed), "post_processing_error.dot");
    const std::string error_dot = read_file("post_processing_error.dot");
    ok &= require_contains(error_dot, "digraph circuit");
    ok &= require_contains(error_dot, "Error:");

    ESE::GraphSizes sizes(1, 2, 1);
    int node_degrees[] = {1, 1};
    ESE::input_node_id inputs[] = {0, 1, 2};
    ESE::CSRGraph graph(sizes, std::span<int>(node_degrees), std::span<ESE::input_node_id>(inputs));
    plot_graph(graph, "post_processing_graph.dot");
    const std::string graph_dot = read_file("post_processing_graph.dot");

    ok &= require_contains(graph_dot, "digraph graph");
    ok &= require_contains(graph_dot, "root_1 -> unit_0");
    ok &= require_contains(graph_dot, "unit_0 -> unit_1");
    ok &= require_contains(graph_dot, "unit_1 -> sink_0");

    if (!ok) {
        return 1;
    }

    std::cout << "post_processing_test passed\n";
    return 0;
}
