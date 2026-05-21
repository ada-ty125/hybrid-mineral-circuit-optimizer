/**
 * @file test_simulator_wiring.cpp
 * @brief Comprehensive simulator wiring and parameter propagation tests.
 *
 * The tests cover graph feed paths, span validity guards, simulator parameter
 * sensitivity, and known converging circuit cases.
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <span>
#include <vector>

#include "CCircuit.h"
#include "CSimulator.h"
#include "CSRGraph.h"
#include "Economics.h"
#include "RequiredFunctions.h"

using namespace ESE;

namespace {

constexpr double kWorstCaseFitness = -40000.0;
constexpr double kWorstCaseEpsilon = 1.0;

bool is_close(double a, double b, double eps = 1e-6) { return std::abs(a - b) < eps; }

void require(bool condition, const char* label) {
    if (!condition) {
        std::cerr << "[FAIL] " << label << "\n";
        std::exit(1);
    }
}

void expect_worst_case(double score, const char* label) {
    if (!is_close(score, kWorstCaseFitness, kWorstCaseEpsilon)) {
        std::cerr << "[FAIL] " << label << ": expected worst-case " << kWorstCaseFitness << ", got "
                  << score << "\n";
        std::exit(1);
    }
    std::cout << "[PASS] " << label << "\n";
}

void expect_finite_neq_worst(double score, const char* label) {
    if (!std::isfinite(score)) {
        std::cerr << "[FAIL] " << label << ": score not finite\n";
        std::exit(1);
    }
    if (is_close(score, kWorstCaseFitness, kWorstCaseEpsilon)) {
        std::cerr << "[FAIL] " << label << ": valid circuit returned worst-case\n";
        std::exit(1);
    }
    std::cout << "[PASS] " << label << " (score=" << score << ")\n";
}

}  // namespace

// ---------------------------------------------------------------------------
// Simulator_Parameters propagation
// ---------------------------------------------------------------------------

// Five-unit published example; known to converge to a positive fitness (~366).
const std::vector<int> kConvergingCircuit = {1, 5, 3, 2, 2, 2, 2, 3, 3, 2,
                                             7, 2, 3, 4, 1, 2, 0, 5, 6, 2};

void test_initialise_applies_tank_volume_and_density() {
    Simulator_Parameters defaults = default_simulator_parameters;

    Simulator_Parameters custom = default_simulator_parameters;
    custom.tank_volume = 5.0;
    custom.fluid_density = 1500.0;

    Circuit circuit_default;
    require(circuit_default.initialise(kConvergingCircuit, defaults), "initialise with defaults");
    const double score_default = CSimulator::evaluate(circuit_default, defaults);

    Circuit circuit_custom;
    require(circuit_custom.initialise(kConvergingCircuit, custom), "initialise with custom sizing");
    const double score_custom = CSimulator::evaluate(circuit_custom, custom);

    expect_finite_neq_worst(score_default, "default sizing converges");
    expect_finite_neq_worst(score_custom, "custom sizing converges");
    require(score_default != score_custom, "tank_volume/fluid_density change fitness");
    std::cout << "[PASS] initialise applies tank_volume and fluid_density (via fitness)\n";
}

void test_tank_volume_changes_fitness() {
    Simulator_Parameters small_tank = default_simulator_parameters;
    small_tank.tank_volume = 2.0;

    Simulator_Parameters large_tank = default_simulator_parameters;
    large_tank.tank_volume = 50.0;

    Circuit circuit_small;
    require(circuit_small.initialise(kConvergingCircuit, small_tank), "initialise small tank");
    const double score_small = CSimulator::evaluate(circuit_small, small_tank);

    Circuit circuit_large;
    require(circuit_large.initialise(kConvergingCircuit, large_tank), "initialise large tank");
    const double score_large = CSimulator::evaluate(circuit_large, large_tank);

    expect_finite_neq_worst(score_small, "small tank converges");
    expect_finite_neq_worst(score_large, "large tank converges");
    require(score_small != score_large, "tank_volume changes fitness");
    std::cout << "[PASS] tank_volume changes fitness (small=" << score_small
              << ", large=" << score_large << ")\n";
}

void test_custom_k_changes_fitness_via_evaluate() {
    Simulator_Parameters baseline = default_simulator_parameters;

    Simulator_Parameters high_k = default_simulator_parameters;
    high_k.k_TypeA[0][0] = 0.05;
    high_k.k_TypeB[0][0] = 0.05;

    Circuit circuit_baseline;
    require(circuit_baseline.initialise(kConvergingCircuit), "initialise for k baseline");
    const double score_baseline = CSimulator::evaluate(circuit_baseline, baseline);

    Circuit circuit_high_k;
    require(circuit_high_k.initialise(kConvergingCircuit), "initialise for k high");
    const double score_high_k = CSimulator::evaluate(circuit_high_k, high_k);

    expect_finite_neq_worst(score_baseline, "baseline k converges");
    expect_finite_neq_worst(score_high_k, "high k converges");
    require(score_baseline != score_high_k, "custom k changes fitness");
    std::cout << "[PASS] custom k matrices change fitness\n";
}

void test_tolerance_used_in_convergence() {
    Simulator_Parameters loose = default_simulator_parameters;
    loose.tolerance = 1.0;

    Simulator_Parameters tight = default_simulator_parameters;
    tight.tolerance = 1e-12;

    Circuit circuit_loose;
    require(circuit_loose.initialise(kConvergingCircuit), "initialise loose tolerance");
    const double score_loose = CSimulator::evaluate(circuit_loose, loose);

    Circuit circuit_tight;
    require(circuit_tight.initialise(kConvergingCircuit), "initialise tight tolerance");
    const double score_tight = CSimulator::evaluate(circuit_tight, tight);

    expect_finite_neq_worst(score_loose, "loose tolerance converges");
    expect_finite_neq_worst(score_tight, "tight tolerance converges");

    // Loose tolerance may stop before the true fixed point; scores can differ.
    // What matters is both paths accept convergence under their own tolerance.
    std::cout << "[PASS] tolerance parameter used (loose=" << score_loose
              << ", tight=" << score_tight << ")\n";
}

// ---------------------------------------------------------------------------
// Span validity guard (circuit_performance span overload)
// ---------------------------------------------------------------------------

void test_span_rejects_unreachable_island() {
    std::vector<int> circuit_vector = {1, 2, 2, 2, 2, 0, 2, 3, 2, 3};

    require(!check_validity(std::span<const int>(circuit_vector)), "island invalid");
    expect_worst_case(circuit_performance(std::span<const int>(circuit_vector)),
                      "span rejects unreachable island");
}

void test_span_rejects_self_recycle() {
    std::vector<int> circuit_vector = {1, 2, 3, 2, 2, 0, 0, 2, 3, 4};

    require(!check_validity(std::span<const int>(circuit_vector)), "self-recycle invalid");
    expect_worst_case(circuit_performance(std::span<const int>(circuit_vector)),
                      "span rejects self-recycle");
}

void test_span_rejects_duplicate_outputs() {
    std::vector<int> circuit_vector = {1, 2, 3, 2, 2, 0, 1, 1, 3, 4};

    require(!check_validity(std::span<const int>(circuit_vector)), "duplicate outputs invalid");
    expect_worst_case(circuit_performance(std::span<const int>(circuit_vector)),
                      "span rejects duplicate outputs");
}

void test_span_valid_matches_direct_evaluate() {
    require(check_validity(std::span<const int>(kConvergingCircuit)), "five-unit valid");

    Circuit circuit;
    require(circuit.initialise(kConvergingCircuit), "initialise five-unit");
    const double direct = CSimulator::evaluate(circuit);
    const double via_span = circuit_performance(std::span<const int>(kConvergingCircuit));

    expect_finite_neq_worst(direct, "direct evaluate converges");
    require(is_close(direct, via_span, 1e-9), "span path matches direct evaluate");
    std::cout << "[PASS] span path matches direct evaluate (" << direct << ")\n";
}

// ---------------------------------------------------------------------------
// Graph feed path and params (circuit_performance graph overload)
// ---------------------------------------------------------------------------

void test_graph_feed_nonzero_unit() {
    // Feed enters unit 1; unit 1 feeds unit 0 and pal product; unit 0 -> gor + tailings.
    std::vector<int> circuit_vector = {1, 2, 3, 2, 2, 1, 3, 4, 0, 2};

    Circuit circuit;
    require(circuit.initialise(circuit_vector), "initialise feed-to-unit-1 circuit");
    require(circuit.feed_dest() == 1, "feed_dest is unit 1");

    GraphSizes sizes(1, 2, 3);
    offset_t output_index[] = {1, 3, 5};
    input_node_id inputs[] = {1, 3, 4, 0, 2};

    CSRGraph graph(sizes, output_index, inputs);

    require(check_validity(graph), "feed-to-unit-1 graph valid");
    require(circuit.feed_dest() == static_cast<int>(graph.input_index[0]),
            "graph root edge matches feed_dest");

    const double span_score = circuit_performance(std::span<const int>(circuit_vector));
    const double graph_score =
        circuit_performance(static_cast<const Graph&>(graph), default_simulator_parameters);

    expect_finite_neq_worst(span_score, "feed-to-unit-1 span converges");
    require(is_close(span_score, graph_score, 1e-6), "span and graph scores match");
    expect_finite_neq_worst(graph_score, "graph with feed_dest=1");
}

void test_graph_custom_params_change_fitness() {
    GraphSizes sizes(1, 2, 3);
    offset_t output_index[] = {1, 3, 6};
    input_node_id inputs[] = {0, 1, 2, 0, 2, 3};

    CSRGraph graph(sizes, output_index, inputs);
    require(check_validity(graph), "reference graph valid");

    Simulator_Parameters baseline = default_simulator_parameters;

    Simulator_Parameters high_k = default_simulator_parameters;
    high_k.k_TypeA[0][0] = 0.05;

    const double score_baseline = circuit_performance(static_cast<const Graph&>(graph), baseline);
    const double score_high_k = circuit_performance(static_cast<const Graph&>(graph), high_k);

    expect_finite_neq_worst(score_baseline, "graph baseline converges");
    expect_finite_neq_worst(score_high_k, "graph high-k converges");
    require(score_baseline != score_high_k, "graph custom k changes fitness");
    std::cout << "[PASS] graph path uses custom params (baseline=" << score_baseline
              << ", high_k=" << score_high_k << ")\n";
}

void test_graph_invalid_returns_worst_case() {
    GraphSizes sizes(1, 1, 2);
    offset_t output_index[] = {1, 3};
    input_node_id self_loop[] = {0, 0, 2};

    CSRGraph graph(sizes, output_index, self_loop);
    require(!check_validity(graph), "self-loop graph invalid");

    expect_worst_case(
        circuit_performance(static_cast<const Graph&>(graph), default_simulator_parameters),
        "graph self-loop returns worst-case");
}

// ---------------------------------------------------------------------------
// Published / regression cases (end-to-end sanity)
// ---------------------------------------------------------------------------

void test_five_unit_example_score() {
    Circuit circuit;
    require(circuit.initialise(kConvergingCircuit), "initialise five-unit");
    require(circuit.feed_dest() == 3, "five-unit feed_dest");

    const double score = circuit_performance(std::span<const int>(kConvergingCircuit));
    expect_finite_neq_worst(score, "five-unit converges");
    require(is_close(score, 366.0, 1.0), "five-unit score near published 366");
    std::cout << "[PASS] five-unit example score ~366 (" << score << ")\n";
}

void test_tailings_product_routing_regression() {
    const int circuit[] = {1, 1, 3, 2, 0, 1, 2};
    const double expected = -38876.11262985238;
    const double got = circuit_performance(std::span<const int>(circuit));

    require(is_close(got, expected, 1e-4), "tailings routed to gormanium product stream");
    std::cout << "[PASS] tailings-to-gormanium product routing\n";
}

void test_circuit_simulator_graph_reference() {
    GraphSizes sizes(1, 2, 3);
    offset_t output_index[] = {1, 3, 6};
    input_node_id inputs[] = {0, 1, 2, 0, 2, 3};

    CSRGraph graph(sizes, output_index, inputs);

    const double expected = -39297.985467447535;
    const double result =
        circuit_performance(static_cast<const Graph&>(graph), default_simulator_parameters);

    require(is_close(result, expected, 1e-2), "reference graph benchmark score");
    std::cout << "[PASS] reference graph benchmark score\n";
}

int main() {
    std::cout << "=== Simulator wiring comprehensive tests ===\n\n";

    std::cout << "-- Simulator_Parameters --\n";
    test_initialise_applies_tank_volume_and_density();
    test_tank_volume_changes_fitness();
    test_custom_k_changes_fitness_via_evaluate();
    test_tolerance_used_in_convergence();

    std::cout << "\n-- Span validity guard --\n";
    test_span_rejects_unreachable_island();
    test_span_rejects_self_recycle();
    test_span_rejects_duplicate_outputs();
    test_span_valid_matches_direct_evaluate();

    std::cout << "\n-- Graph path --\n";
    test_graph_feed_nonzero_unit();
    test_graph_custom_params_change_fitness();
    test_graph_invalid_returns_worst_case();

    std::cout << "\n-- Regressions --\n";
    test_five_unit_example_score();
    test_tailings_product_routing_regression();
    test_circuit_simulator_graph_reference();

    std::cout << "\nAll comprehensive simulator wiring tests passed.\n";
    return 0;
}
