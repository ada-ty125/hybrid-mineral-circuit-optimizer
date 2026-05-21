/**
 * @file Visualization.cpp
 * @brief Graphviz DOT visualisation utilities for circuit vectors and ESE graphs.
 *
 * This file contains the post-processing visualisation implementation used to
 * inspect candidate separation circuits. It writes DOT text files that can be
 * rendered by Graphviz into images for reports, debugging, and manual review.
 */

#include "CSRGraph.h"
#include "CCircuit.h"
#include "CSimulator.h"
#include "Economics.h"
#include "RequiredFunctions.h"

#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

bool requests_gif(const std::filesystem::path& path) { return lower_extension(path) == ".gif"; }

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

struct ParsedVisualCircuit {
    std::size_t num_inputs = 0;
    std::size_t num_units = 0;
    std::size_t num_products = 0;
    int feed_destination = 0;
    std::vector<int> output_counts;
    std::vector<std::vector<int>> destinations;
};

struct FlowEdge {
    std::string id;
    std::string source_id;
    std::string target_id;
    std::string label;
    std::vector<double> flows;
    double total_flow = 0.0;
};

struct VisualisationData {
    ParsedVisualCircuit parsed;
    std::vector<std::string> component_names;
    std::vector<double> feed_rates;
    std::vector<std::vector<double>> final_products;
    std::vector<double> final_tailings;
    std::vector<FlowEdge> edges;
    double score = 0.0;
    std::size_t type_a_count = 0;
    std::size_t type_b_count = 0;
};

std::string json_escape(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size() + 2);
    escaped += '"';
    for (char c : text) {
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += c;
                break;
        }
    }
    escaped += '"';
    return escaped;
}

double flow_sum(const std::vector<double>& values) {
    return std::accumulate(values.begin(), values.end(), 0.0);
}

double component_at(const std::vector<double>& values, std::size_t index) {
    return index < values.size() ? values[index] : 0.0;
}

double grade_percent(const std::vector<double>& stream, std::size_t component) {
    const double total = flow_sum(stream);
    if (total <= 0.0) {
        return 0.0;
    }
    return 100.0 * component_at(stream, component) / total;
}

double recovery_percent(const std::vector<double>& stream, std::size_t component,
                        const std::vector<double>& feed) {
    const double feed_component = component_at(feed, component);
    if (feed_component <= 0.0) {
        return 0.0;
    }
    return 100.0 * component_at(stream, component) / feed_component;
}

std::string component_colour(std::size_t component) {
    static const std::vector<std::string> colours = {"#d6a100", "#2878c8", "#6f7b85", "#17a589",
                                                     "#8e44ad"};
    return colours[component % colours.size()];
}

std::vector<double> sized_flow(std::vector<double> values, std::size_t size) {
    values.resize(size, 0.0);
    return values;
}

bool parse_visual_circuit(std::span<const int> values, ParsedVisualCircuit& parsed,
                          std::string& error) {
    if (values.size() < 5) {
        error = "circuit vector is too short";
        return false;
    }
    if (values[0] < 0 || values[1] < 0 || values[2] < 0) {
        error = "negative circuit size field";
        return false;
    }

    parsed.num_inputs = static_cast<std::size_t>(values[0]);
    parsed.num_units = static_cast<std::size_t>(values[1]);
    parsed.num_products = static_cast<std::size_t>(values[2]);

    if (parsed.num_inputs != 1) {
        error = "only one input stream is supported";
        return false;
    }
    if (parsed.num_products != 3) {
        error = "expected three product streams";
        return false;
    }
    if (values.size() < 3 + parsed.num_units + 1) {
        error = "missing unit output counts or feed target";
        return false;
    }

    parsed.output_counts.assign(parsed.num_units, 0);
    std::size_t output_count_sum = 0;
    for (std::size_t unit = 0; unit < parsed.num_units; ++unit) {
        const int output_count = values[3 + unit];
        if (output_count != 2 && output_count != 3) {
            error = "unit output count must be 2 or 3";
            return false;
        }
        parsed.output_counts[unit] = output_count;
        output_count_sum += static_cast<std::size_t>(output_count);
    }

    const std::size_t expected_size = 3 + parsed.num_units + 1 + output_count_sum;
    if (values.size() != expected_size) {
        error = "circuit vector length does not match header";
        return false;
    }

    const std::size_t feed_index = 3 + parsed.num_units;
    parsed.feed_destination = values[feed_index];
    parsed.destinations.assign(parsed.num_units, {});
    std::size_t destination_index = feed_index + 1;
    for (std::size_t unit = 0; unit < parsed.num_units; ++unit) {
        auto& unit_destinations = parsed.destinations[unit];
        unit_destinations.reserve(static_cast<std::size_t>(parsed.output_counts[unit]));
        for (int output_id = 0; output_id < parsed.output_counts[unit]; ++output_id) {
            unit_destinations.push_back(values[destination_index++]);
        }
    }
    return true;
}

bool build_visualisation_data(std::span<const int> values, VisualisationData& data,
                              std::string& error) {
    if (!parse_visual_circuit(values, data.parsed, error)) {
        return false;
    }

    const Simulator_Parameters& params = default_simulator_parameters;
    const std::size_t component_count = static_cast<std::size_t>(params.n_components);
    data.component_names = params.component_names;
    data.component_names.resize(component_count);
    for (std::size_t comp = 0; comp < component_count; ++comp) {
        if (data.component_names[comp].empty()) {
            data.component_names[comp] = "Component " + std::to_string(comp);
        }
    }

    Circuit circuit;
    const bool valid_for_simulation = circuit.initialise(values, params);
    data.score =
        cuprite::worst_case_value(params.input_feed_rates.back(), cuprite::default_economics);
    if (valid_for_simulation) {
        data.score = CSimulator::evaluate(circuit, params);
    }

    data.feed_rates = params.input_feed_rates;
    data.feed_rates.resize(component_count, 0.0);

    data.final_products =
        valid_for_simulation ? circuit.final_products : std::vector<std::vector<double>>{};
    data.final_products.resize(data.parsed.num_products, std::vector<double>(component_count, 0.0));
    for (auto& product : data.final_products) {
        product.resize(component_count, 0.0);
    }

    data.final_tailings = valid_for_simulation ? circuit.final_tailings : std::vector<double>{};
    data.final_tailings.resize(component_count, 0.0);

    data.edges.clear();
    data.edges.push_back({"edge_feed", "feed",
                          circuit_target_name(data.parsed.feed_destination, data.parsed.num_units),
                          "feed", data.feed_rates, flow_sum(data.feed_rates)});

    for (std::size_t unit = 0; unit < data.parsed.num_units; ++unit) {
        for (std::size_t output_id = 0; output_id < data.parsed.destinations[unit].size();
             ++output_id) {
            std::vector<double> flows(component_count, 0.0);
            if (valid_for_simulation && unit < circuit.units.size()) {
                const CUnit& sim_unit = circuit.units[unit];
                if (output_id + 1 < static_cast<std::size_t>(sim_unit.n_outputs) &&
                    output_id < sim_unit.concentrate.size()) {
                    flows = sized_flow(sim_unit.concentrate[output_id], component_count);
                } else if (output_id < static_cast<std::size_t>(sim_unit.n_outputs)) {
                    flows = sized_flow(sim_unit.tails, component_count);
                }
            }

            const int destination = data.parsed.destinations[unit][output_id];
            const std::string edge_id =
                "edge_" + std::to_string(unit) + "_" + std::to_string(output_id);
            data.edges.push_back(
                {edge_id, circuit_node_name(unit),
                 circuit_target_name(destination, data.parsed.num_units),
                 output_label(static_cast<std::size_t>(data.parsed.output_counts[unit]), output_id),
                 flows, flow_sum(flows)});
        }
    }

    data.type_a_count = 0;
    data.type_b_count = 0;
    for (int output_count : data.parsed.output_counts) {
        if (output_count == 2) {
            ++data.type_a_count;
        } else if (output_count == 3) {
            ++data.type_b_count;
        }
    }
    return true;
}

void write_json_number_array(std::ostream& output, const std::vector<double>& values) {
    output << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            output << ", ";
        }
        output << std::setprecision(12) << values[i];
    }
    output << "]";
}

void write_json_number_matrix(std::ostream& output,
                              const std::vector<std::vector<double>>& values) {
    output << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            output << ", ";
        }
        write_json_number_array(output, values[i]);
    }
    output << "]";
}

bool write_animation_payload_json(const VisualisationData& data,
                                  const std::filesystem::path& payload_path) {
    std::ofstream output(payload_path);
    if (!output) {
        std::cerr << "Could not open animation payload file: " << payload_path << "\n";
        return false;
    }

    output << "{\n";
    output << "  \"components\": [\n";
    for (std::size_t comp = 0; comp < data.component_names.size(); ++comp) {
        output << "    {\"name\": " << json_escape(data.component_names[comp])
               << ", \"colour\": " << json_escape(component_colour(comp)) << "}";
        output << (comp + 1 == data.component_names.size() ? "\n" : ",\n");
    }
    output << "  ],\n";
    output << "  \"feed_rates\": ";
    write_json_number_array(output, data.feed_rates);
    output << ",\n";

    output << "  \"nodes\": [\n";
    output << "    {\"id\": \"feed\", \"kind\": \"feed\", \"label\": \"Feed\"}";
    for (std::size_t unit = 0; unit < data.parsed.num_units; ++unit) {
        output << ",\n    {\"id\": " << json_escape(circuit_node_name(unit))
               << ", \"kind\": \"unit\", \"label\": " << json_escape("Unit " + std::to_string(unit))
               << ", \"unit_index\": " << unit << ", \"unit_type\": "
               << json_escape(data.parsed.output_counts[unit] == 3 ? "B" : "A") << "}";
    }
    const std::vector<std::string> product_ids = {"palusznium_product", "gormanium_product",
                                                  "tailings_product"};
    const std::vector<std::string> product_labels = {"Palusznium product", "Gormanium product",
                                                     "Tailings"};
    for (std::size_t product = 0; product < data.parsed.num_products; ++product) {
        output << ",\n    {\"id\": " << json_escape(product_ids[product])
               << ", \"kind\": \"product\", \"label\": " << json_escape(product_labels[product])
               << ", \"product_index\": " << product << "}";
    }
    output << "\n  ],\n";

    output << "  \"edges\": [\n";
    for (std::size_t i = 0; i < data.edges.size(); ++i) {
        const FlowEdge& edge = data.edges[i];
        output << "    {\"id\": " << json_escape(edge.id)
               << ", \"source\": " << json_escape(edge.source_id)
               << ", \"target\": " << json_escape(edge.target_id)
               << ", \"label\": " << json_escape(edge.label) << ", \"flows\": ";
        write_json_number_array(output, edge.flows);
        output << ", \"total_flow\": " << std::setprecision(12) << edge.total_flow << "}";
        output << (i + 1 == data.edges.size() ? "\n" : ",\n");
    }
    output << "  ],\n";

    output << "  \"results\": {\n";
    output << "    \"score\": " << std::setprecision(12) << data.score << ",\n";
    output << "    \"type_a_count\": " << data.type_a_count << ",\n";
    output << "    \"type_b_count\": " << data.type_b_count << ",\n";
    output << "    \"pal_recovery\": "
           << recovery_percent(data.final_products[0], 0, data.feed_rates) << ",\n";
    output << "    \"pal_grade\": " << grade_percent(data.final_products[0], 0) << ",\n";
    output << "    \"gor_recovery\": "
           << recovery_percent(data.final_products[1], 1, data.feed_rates) << ",\n";
    output << "    \"gor_grade\": " << grade_percent(data.final_products[1], 1) << ",\n";
    output << "    \"products\": ";
    write_json_number_matrix(output, data.final_products);
    output << ",\n";
    output << "    \"tailings\": ";
    write_json_number_array(output, data.final_tailings);
    output << "\n  }\n";
    output << "}\n";

    if (!output) {
        std::cerr << "Could not write animation payload file: " << payload_path << "\n";
        return false;
    }
    return true;
}

std::filesystem::path animation_script_path() {
    std::vector<std::filesystem::path> candidates;
#ifdef CIRCUIT_PROJECT_SOURCE_DIR
    candidates.push_back(std::filesystem::path(CIRCUIT_PROJECT_SOURCE_DIR) / "tools" /
                         "animate_circuit.py");
#endif
    candidates.push_back(std::filesystem::current_path() / "tools" / "animate_circuit.py");
    candidates.push_back(std::filesystem::current_path() / ".." / "tools" / "animate_circuit.py");
    candidates.push_back(std::filesystem::current_path() / ".." / ".." / "tools" /
                         "animate_circuit.py");

    for (const auto& candidate : candidates) {
        std::error_code error;
        if (std::filesystem::exists(candidate, error)) {
            return candidate;
        }
    }
    return {};
}

std::filesystem::path temporary_payload_path_for(const std::filesystem::path& output_path) {
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path payload_path = std::filesystem::temp_directory_path();
    payload_path /=
        output_path.stem().string() + "_circuit_animation_" + std::to_string(tick) + ".json";
    return payload_path;
}

bool render_python_animation(const std::filesystem::path& payload_path,
                             const std::filesystem::path& output_path) {
    const std::filesystem::path script_path = animation_script_path();
    if (script_path.empty()) {
        std::cerr << "Could not find Python animation script tools/animate_circuit.py\n";
        return false;
    }

    const std::filesystem::path mpl_config_dir =
        std::filesystem::temp_directory_path() / "cuprite_matplotlib";
    const std::filesystem::path font_cache_dir =
        std::filesystem::temp_directory_path() / "cuprite_font_cache";
    std::error_code error;
    std::filesystem::create_directories(mpl_config_dir, error);
    std::filesystem::create_directories(font_cache_dir, error);

    const std::string command = "MPLCONFIGDIR=" + shell_quote(mpl_config_dir.string()) +
                                " XDG_CACHE_HOME=" + shell_quote(font_cache_dir.string()) +
                                " python3 " + shell_quote(script_path.string()) + " " +
                                shell_quote(payload_path.string()) + " " +
                                shell_quote(output_path.string());
    const int status = std::system(command.c_str());
    if (status != 0) {
        std::cerr << "Could not render animated GIF visualization: " << output_path << "\n";
        return false;
    }
    return true;
}

void write_python_animation(std::span<const int> values, const char* filename) {
    const std::filesystem::path output_path(filename);
    VisualisationData data;
    std::string error;
    if (!build_visualisation_data(values, data, error)) {
        std::filesystem::path fallback_path = output_path;
        fallback_path.replace_extension(".dot");
        std::ofstream output(fallback_path);
        if (output) {
            write_error_dot(output, error);
        }
        std::cerr << "Could not build GIF animation data. Wrote DOT error output to "
                  << fallback_path << "\n";
        return;
    }

    const std::filesystem::path payload_path = temporary_payload_path_for(output_path);
    if (!write_animation_payload_json(data, payload_path)) {
        return;
    }

    render_python_animation(payload_path, output_path);

    std::error_code remove_error;
    std::filesystem::remove(payload_path, remove_error);
    if (remove_error) {
        std::cerr << "Could not remove temporary animation payload file: " << payload_path << "\n";
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
    const std::filesystem::path output_path(filename);
    if (requests_gif(output_path)) {
        write_python_animation(values, filename);
        return;
    }

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
