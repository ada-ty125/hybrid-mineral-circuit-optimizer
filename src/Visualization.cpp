/**
 * @file Visualization.cpp
 * @brief Graphviz DOT visualisation utilities for circuit vectors and ESE graphs.
 *
 * This file contains the post-processing visualisation implementation used to
 * inspect candidate separation circuits. It writes DOT text files that can be
 * rendered by Graphviz into images for reports, debugging, and manual review.
 */

#include "CSRGraph.h"
#include "RequiredFunctions.h"

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

namespace {

/**
 * @brief Write a minimal DOT graph that reports why visualisation failed.
 *
 * This helper is used for malformed circuit vectors. Emitting a DOT file even
 * on invalid input keeps the visualisation step inspectable and avoids silent
 * failures in post-processing workflows.
 *
 * @param output Open output stream for the DOT file.
 * @param message Human-readable validation error to display in the graph.
 */
void write_error_dot(std::ostream& output, const std::string& message) {
    output << "digraph circuit {\n";
    output << "  graph [rankdir=LR];\n";
    output << "  error [shape=box, color=red, label=\"Error: " << message << "\"];\n";
    output << "}\n";
}

/**
 * @brief Return the DOT node identifier used for a circuit processing unit.
 *
 * @param node_id Zero-based processing unit index.
 * @return Stable DOT identifier in the form `unit_<id>`.
 */
std::string circuit_node_name(std::size_t node_id) { return "unit_" + std::to_string(node_id); }

/**
 * @brief Map a circuit-vector destination value to a DOT node identifier.
 *
 * Circuit vectors encode unit destinations as integer indices. Values below
 * `num_units` target another processing unit. The three values starting at
 * `num_units` represent the Palusznium, Gormanium, and tailings product sinks.
 * Any out-of-range destination is converted to an `invalid_*` node name so the
 * generated graph still exposes the problematic edge.
 *
 * @param destination Encoded circuit-vector destination.
 * @param num_units Number of processing units in the circuit.
 * @return DOT node identifier for the target node.
 */
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

/**
 * @brief Return a human-readable edge label for a unit output stream.
 *
 * Type A units have two outputs: concentrate and tailings. Type B units have
 * three outputs: Palusznium concentrate, Gormanium concentrate, and tailings.
 *
 * @param output_count Number of outputs for the source unit.
 * @param output_index Zero-based output index on that unit.
 * @return Label to attach to the DOT edge.
 */
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

/**
 * @brief Return the display colour for an output stream edge.
 *
 * The colour scheme mirrors the stream semantics: Palusznium-rich streams use
 * gold, Gormanium-rich streams use blue, and tailings use grey.
 *
 * @param output_count Number of outputs for the source unit.
 * @param output_index Zero-based output index on that unit.
 * @return Graphviz colour name for the DOT edge.
 */
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

/**
 * @brief Convert an ESE graph output node id to a DOT node identifier.
 *
 * In the ESE graph representation, root/input nodes are encoded as negative
 * ids, while dynamic processing units use non-negative ids.
 *
 * @param output_node ESE graph node id.
 * @return DOT node identifier for a root or processing unit.
 */
std::string graph_output_node_name(ESE::node_id output_node) {
    if (output_node < 0) {
        return "root_" + std::to_string(-output_node);
    }
    return "unit_" + std::to_string(output_node);
}

/**
 * @brief Convert an ESE graph target id to a DOT node identifier.
 *
 * Targets below `num_units` refer to dynamic processing units. Targets at or
 * above `num_units` refer to sink nodes, offset by the number of units.
 *
 * @param target ESE graph input target id.
 * @param num_units Number of dynamic processing units in the graph.
 * @return DOT node identifier for a unit, sink, or unfilled target.
 */
std::string graph_target_node_name(ESE::input_node_id target, std::size_t num_units) {
    if (target == ESE::UNFILLED_INPUT) {
        return "unfilled";
    }
    if (target < num_units) {
        return "unit_" + std::to_string(target);
    }
    return "sink_" + std::to_string(target - num_units);
}

std::string lower_extension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    for (char& c : extension) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return extension;
}

bool requests_png(const std::filesystem::path& path) { return lower_extension(path) == ".png"; }

std::string shell_quote(std::string_view value) {
    std::string quoted = "'";
    for (char c : value) {
        if (c == '\'') {
            quoted += "'\\''";
        } else {
            quoted += c;
        }
    }
    quoted += "'";
    return quoted;
}

std::filesystem::path temporary_dot_path_for(const std::filesystem::path& output_path) {
    std::filesystem::path dot_path = output_path;
    dot_path += ".dot";
    return dot_path;
}

bool render_dot_to_png(const std::filesystem::path& dot_path,
                       const std::filesystem::path& png_path) {
    const std::string command =
        "dot -Tpng " + shell_quote(dot_path.string()) + " -o " + shell_quote(png_path.string());
    const int status = std::system(command.c_str());
    if (status != 0) {
        std::cerr << "Could not render PNG visualization with Graphviz: " << png_path
                  << "\nDOT source left at: " << dot_path << "\n";
        return false;
    }
    return true;
}

template <typename Writer>
void write_visualization_file(const char* filename, Writer write_dot, const char* description) {
    const std::filesystem::path output_path(filename);
    const bool png_output = requests_png(output_path);
    const std::filesystem::path dot_path =
        png_output ? temporary_dot_path_for(output_path) : output_path;

    std::ofstream output(dot_path);
    if (!output) {
        std::cerr << "Could not open " << description << " visualization output file: " << dot_path
                  << "\n";
        return;
    }

    write_dot(output);
    output.close();
    if (!output) {
        std::cerr << "Could not write " << description << " visualization output file: " << dot_path
                  << "\n";
        return;
    }

    if (!png_output) {
        return;
    }

    if (!render_dot_to_png(dot_path, output_path)) {
        return;
    }

    std::error_code error;
    std::filesystem::remove(dot_path, error);
    if (error) {
        std::cerr << "Could not remove temporary DOT file: " << dot_path << "\n";
    }
}

}  // namespace

/**
 * @brief Write a Graphviz DOT visualisation for an encoded circuit vector.
 *
 * The expected vector layout is:
 * `[num_inputs, num_units, num_products, unit_output_counts..., feed_dest,
 * unit_output_destinations...]`.
 *
 * This implementation supports the project circuit convention of one feed
 * stream and three product streams. Unit output counts must be either two
 * (Type A) or three (Type B). The generated DOT file distinguishes feed,
 * processing units, product sinks, stream types, and self-recycle edges.
 *
 * If the vector is malformed, the function writes an error DOT graph instead
 * of throwing. File-open failures are reported to stderr and produce no output.
 *
 * @param values Circuit vector to visualise.
 * @param filename Path to the DOT file to write.
 */
void plot_span(std::span<const int> const values, const char* filename) {
    write_visualization_file(
        filename,
        [values](std::ostream& output) {
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
                       << "\", label=\"Unit " << unit << "\\nType "
                       << (output_count == 2 ? "A" : "B") << "\"];\n";
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
                           << (self_recycle ? "red" : edge_color(output_count, output_id))
                           << "\"];\n";
                }
            }

            output << "}\n";
        },
        "circuit");
}

/**
 * @brief Write a Graphviz DOT visualisation for an ESE graph.
 *
 * Root nodes are drawn as feed inputs, dynamic nodes as processing units, and
 * sink nodes as outputs. Edges follow the target structure exposed by the ESE
 * graph API. This function is useful after converting a circuit vector into
 * the graph representation used by validity checking and simulation.
 *
 * File-open failures are reported to stderr and produce no output.
 *
 * @param graph ESE graph to visualise.
 * @param filename Path to the DOT file to write.
 */
void plot_graph(const Graph& graph, const char* filename) {
    write_visualization_file(
        filename,
        [&graph](std::ostream& output) {
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
                       << graph_target_node_name(target, graph.n_dynamic_nodes)
                       << " [label=\"feed\"];\n";
            }
            for (std::size_t unit = 0; unit < graph.n_dynamic_nodes; ++unit) {
                for (std::size_t edge = 0; edge < graph.get_degree(unit); ++edge) {
                    const ESE::input_node_id target = graph.node_targets(unit, edge);
                    output << "  unit_" << unit << " -> "
                           << graph_target_node_name(target, graph.n_dynamic_nodes)
                           << " [label=\"output " << edge << "\"];\n";
                }
            }

            output << "}\n";
        },
        "graph");
}
