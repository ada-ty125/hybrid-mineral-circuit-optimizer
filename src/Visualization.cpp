#include "CSRGraph.h"
#include "RequiredFunctions.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <span>
#include <string>

namespace {

void write_error_dot(std::ofstream& output, const std::string& message) {
    output << "digraph circuit {\n";
    output << "  graph [rankdir=LR];\n";
    output << "  error [shape=box, color=red, label=\"Error: " << message << "\"];\n";
    output << "}\n";
}

std::string circuit_node_name(std::size_t node_id) { return "unit_" + std::to_string(node_id); }

std::string circuit_target_name(int destination, std::size_t num_units) {
    if (destination < 0) {
        return "invalid_" + std::to_string(-destination);
    }

    const auto dest = static_cast<std::size_t>(destination);
    if (dest < num_units) {
        return circuit_node_name(dest);
    }
    if (dest == num_units) {
        return "palusznium_product";
    }
    if (dest == num_units + 1) {
        return "gormanium_product";
    }
    if (dest == num_units + 2) {
        return "tailings_product";
    }
    return "invalid_" + std::to_string(dest);
}

std::string output_label(std::size_t output_count, std::size_t output_index) {
    if (output_count == 2) {
        return output_index == 0 ? "concentrate" : "tailings";
    }
    if (output_count == 3) {
        if (output_index == 0) {
            return "palusznium concentrate";
        }
        if (output_index == 1) {
            return "gormanium concentrate";
        }
        return "tailings";
    }
    return "output " + std::to_string(output_index);
}

std::string edge_color(std::size_t output_count, std::size_t output_index) {
    if (output_count == 2) {
        return output_index == 0 ? "goldenrod" : "gray45";
    }
    if (output_count == 3) {
        if (output_index == 0) {
            return "goldenrod";
        }
        if (output_index == 1) {
            return "royalblue";
        }
        return "gray45";
    }
    return "black";
}

std::string graph_output_node_name(ESE::node_id output_node) {
    if (output_node < 0) {
        return "root_" + std::to_string(-output_node);
    }
    return "unit_" + std::to_string(output_node);
}

std::string graph_target_node_name(ESE::input_node_id target, std::size_t num_units) {
    if (target == ESE::UNFILLED_INPUT) {
        return "unfilled";
    }
    if (target < num_units) {
        return "unit_" + std::to_string(target);
    }
    return "sink_" + std::to_string(target - num_units);
}

}  // namespace

void plot_span(std::span<const int> const values, const char* filename) {
    std::ofstream output(filename);
    if (!output) {
        std::cerr << "Could not open visualization output file: " << filename << "\n";
        return;
    }

    if (values.size() < 5) {
        write_error_dot(output, "circuit vector is too short");
        return;
    }

    if (values[0] < 0 || values[1] < 0 || values[2] < 0) {
        write_error_dot(output, "negative circuit size field");
        return;
    }

    const auto num_inputs = static_cast<std::size_t>(values[0]);
    const auto num_units = static_cast<std::size_t>(values[1]);
    const auto num_products = static_cast<std::size_t>(values[2]);

    if (num_inputs != 1) {
        write_error_dot(output, "only one input stream is supported");
        return;
    }
    if (num_products != 3) {
        write_error_dot(output, "expected three product streams");
        return;
    }
    if (values.size() < 3 + num_units + 1) {
        write_error_dot(output, "missing unit output counts or feed target");
        return;
    }

    std::size_t output_count_sum = 0;
    for (std::size_t unit = 0; unit < num_units; ++unit) {
        const int output_count = values[3 + unit];
        if (output_count != 2 && output_count != 3) {
            write_error_dot(output, "unit output count must be 2 or 3");
            return;
        }
        output_count_sum += static_cast<std::size_t>(output_count);
    }

    const std::size_t expected_size = 3 + num_units + 1 + output_count_sum;
    if (values.size() != expected_size) {
        write_error_dot(output, "circuit vector length does not match header");
        return;
    }

    const std::size_t feed_index = 3 + num_units;
    std::size_t destination_index = feed_index + 1;

    output << "digraph circuit {\n";
    output << "  graph [rankdir=LR, bgcolor=\"white\"];\n";
    output << "  node [fontname=\"Helvetica\", fontsize=10];\n";
    output << "  edge [fontname=\"Helvetica\", fontsize=9];\n\n";

    output << "  feed [shape=oval, style=filled, fillcolor=\"palegreen\", "
              "label=\"Feed\\nP: 8 kg/s\\nG: 12 kg/s\\nW: 80 kg/s\"];\n";
    for (std::size_t unit = 0; unit < num_units; ++unit) {
        const int output_count = values[3 + unit];
        const char* fill = output_count == 2 ? "lightgoldenrod1" : "lightblue";
        output << "  " << circuit_node_name(unit)
               << " [shape=box, style=\"rounded,filled\", fillcolor=\"" << fill
               << "\", label=\"Unit " << unit << "\\nType " << (output_count == 2 ? "A" : "B")
               << "\"];\n";
    }
    output << "  palusznium_product [shape=oval, style=filled, "
              "fillcolor=\"gold\", label=\"Palusznium\\nproduct\"];\n";
    output << "  gormanium_product [shape=oval, style=filled, "
              "fillcolor=\"lightskyblue\", label=\"Gormanium\\nproduct\"];\n";
    output << "  tailings_product [shape=oval, style=filled, "
              "fillcolor=\"lightgray\", label=\"Tailings\"];\n\n";

    const int feed_destination = values[feed_index];
    output << "  feed -> " << circuit_target_name(feed_destination, num_units)
           << " [label=\"feed\", color=\"forestgreen\"];\n";

    for (std::size_t unit = 0; unit < num_units; ++unit) {
        const auto output_count = static_cast<std::size_t>(values[3 + unit]);
        for (std::size_t output_id = 0; output_id < output_count; ++output_id) {
            const int destination = values[destination_index++];
            const bool self_recycle =
                destination >= 0 && static_cast<std::size_t>(destination) == unit;
            output << "  " << circuit_node_name(unit) << " -> "
                   << circuit_target_name(destination, num_units) << " [label=\""
                   << output_label(output_count, output_id) << "\", color=\""
                   << (self_recycle ? "red" : edge_color(output_count, output_id)) << "\"];\n";
        }
    }

    output << "}\n";
}

void plot_graph(const Graph& graph, const char* filename) {
    std::ofstream output(filename);
    if (!output) {
        std::cerr << "Could not open graph visualization output file: " << filename << "\n";
        return;
    }

    output << "digraph graph {\n";
    output << "  graph [rankdir=LR, bgcolor=\"white\"];\n";
    output << "  node [fontname=\"Helvetica\", fontsize=10];\n";
    output << "  edge [fontname=\"Helvetica\", fontsize=9];\n\n";

    for (ESE::node_id root : graph.root_nodes()) {
        output << "  " << graph_output_node_name(root)
               << " [shape=oval, style=filled, fillcolor=\"palegreen\", "
                  "label=\"Input "
               << (-root) << "\"];\n";
    }
    for (std::size_t unit = 0; unit < graph.n_dynamic_nodes; ++unit) {
        output << "  unit_" << unit
               << " [shape=box, style=\"rounded,filled\", "
                  "fillcolor=\"lightgoldenrod1\", label=\"Unit "
               << unit << "\"];\n";
    }
    for (std::size_t sink = 0; sink < graph.n_sink_nodes; ++sink) {
        output << "  sink_" << sink
               << " [shape=oval, style=filled, fillcolor=\"lightgray\", "
                  "label=\"Sink "
               << sink << "\"];\n";
    }
    output << "\n";

    for (ESE::node_id root : graph.root_nodes()) {
        const ESE::input_node_id target = graph.node_targets(root, 0);
        output << "  " << graph_output_node_name(root) << " -> "
               << graph_target_node_name(target, graph.n_dynamic_nodes) << " [label=\"feed\"];\n";
    }
    for (std::size_t unit = 0; unit < graph.n_dynamic_nodes; ++unit) {
        for (std::size_t edge = 0; edge < graph.get_degree(unit); ++edge) {
            const ESE::input_node_id target = graph.node_targets(unit, edge);
            output << "  unit_" << unit << " -> "
                   << graph_target_node_name(target, graph.n_dynamic_nodes) << " [label=\"output "
                   << edge << "\"];\n";
        }
    }

    output << "}\n";
}
