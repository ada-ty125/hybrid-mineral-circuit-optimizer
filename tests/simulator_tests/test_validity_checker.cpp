#include <iostream>
#include <vector>
#include <span>

#include "CSRGraph.h"
#include "CUnit.h"
#include "CCircuit.h"

using namespace ESE;

/**
 * @brief Unit Test Module for Circuit Validity Checking
 * This file verifies that check_validity correctly approves legal topologies
 * and ruthlessly terminates pathological or broken mineral circuits.
 */
int main(int argc, char* argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning

    std::cout << "==================================================\n";
    std::cout << "   STARTING INTEGRATION TESTS FOR TASK 3          \n";
    std::cout << "==================================================\n\n";

    // Standard baseline sizes for a minimal operational circuit:
    // 1 Root Node (Feed), 1 Dynamic Node (Separation Unit), 2 Sink Nodes (Outlets)
    GraphSizes basic_sizes(1, 1, 2);

    // CRITICAL FIX: Match the inner template expected by CSRGraph.h's node_degrees
    std::size_t single_unit_degree[] = {2};

    // -------------------------------------------------------------------------
    // TEST 1: The Golden Standard (Valid Circuit)
    // Feed -> Unit 0; Unit 0 Concentrate -> Outlet(1); Unit 0 Tailings -> Outlet(2)
    // -------------------------------------------------------------------------
    std::cout << "[TEST 1] Testing a fully legal baseline circuit...\n";
    input_node_id valid_connections[] = {0, 1, 2};

    // THE ABSOLUTE REPAIR: Explicitly build dynamic_extent spans to strictly avoid compiler CTAD
    // errors.
    CSRGraph graph_valid(basic_sizes.n_root_nodes, basic_sizes.n_dynamic_nodes,
                         basic_sizes.n_sink_nodes,
                         std::span<std::size_t, std::dynamic_extent>(single_unit_degree, 1),
                         std::span<input_node_id, std::dynamic_extent>(valid_connections, 3));

    // Hardened flush logic using std::endl to guarantee terminal output before execution
    std::cerr << "--- [DEBUGGING CSRGRAPH INDEXES] ---" << std::endl;
    std::cerr << "Total Dynamic Nodes (Units): " << graph_valid.n_dynamic_nodes << std::endl;
    if (graph_valid.input_index != nullptr) {
        std::cerr << "Feed (Root 0) points to node ID: " << graph_valid.input_index[0] << std::endl;
    } else {
        std::cerr << "WARNING: graph_valid.input_index is NULLPTR!" << std::endl;
    }
    std::cerr << "Unit 0 Output 0 points to: " << graph_valid.node_targets(0, 0) << std::endl;
    std::cerr << "Unit 0 Output 1 points to: " << graph_valid.node_targets(0, 1) << std::endl;
    std::cerr << "------------------------------------" << std::endl;

    if (check_validity(graph_valid)) {
        std::cout << "-> SUCCESS: Valid circuit correctly APPROVED.\n\n";
    } else {
        std::cerr << "-> FATAL ERROR: Valid circuit was incorrectly REJECTED!\n";
        return 1;  // Test failed, collapse the pipeline
    }

    // -------------------------------------------------------------------------
    // TEST 2: Rule 4 Violation (Identical Destinations / Merge into same node)
    // Feed -> Unit 0; Unit 0 sends BOTH streams to Outlet(1). This is useless.
    // -------------------------------------------------------------------------
    std::cout << "[TEST 2] Testing identical destinations (Condition 4)...\n";
    input_node_id identical_dest_connections[] = {0, 1, 1};
    CSRGraph graph_identical(
        basic_sizes.n_root_nodes, basic_sizes.n_dynamic_nodes, basic_sizes.n_sink_nodes,
        std::span<std::size_t, std::dynamic_extent>(single_unit_degree, 1),
        std::span<input_node_id, std::dynamic_extent>(identical_dest_connections, 3));

    if (!check_validity(graph_identical)) {
        std::cout << "-> SUCCESS: Invalid identical-destination circuit correctly REJECTED.\n\n";
    } else {
        std::cerr << "-> FATAL ERROR: Merged-stream circuit mistakenly APPROVED!\n";
        return 1;
    }

    // -------------------------------------------------------------------------
    // TEST 3: Rule 3 Violation (Self-Recycle Loop / Infinite Feedback Loop)
    // Feed -> Unit 0; Unit 0 loops one stream back into its own feed inlet.
    // -------------------------------------------------------------------------
    std::cout << "[TEST 3] Testing absolute self-recycle loops (Condition 3)...\n";
    input_node_id self_recycle_connections[] = {0, 0, 2};
    CSRGraph graph_self_loop(
        basic_sizes.n_root_nodes, basic_sizes.n_dynamic_nodes, basic_sizes.n_sink_nodes,
        std::span<std::size_t, std::dynamic_extent>(single_unit_degree, 1),
        std::span<input_node_id, std::dynamic_extent>(self_recycle_connections, 3));

    if (!check_validity(graph_self_loop)) {
        std::cout << "-> SUCCESS: Pathological self-recycle circuit correctly REJECTED.\n\n";
    } else {
        std::cerr << "-> FATAL ERROR: Dangerous self-loop circuit mistakenly APPROVED!\n";
        return 1;
    }

    // -------------------------------------------------------------------------
    // TEST 4: Rule 1 Violation (Isolated Island / Unreachable from Feed)
    // We create a circuit with 2 units. Feed enters Unit 0, but Unit 1 is standalone.
    // -------------------------------------------------------------------------
    std::cout << "[TEST 4] Testing structural isolated islands (Condition 1)...\n";
    GraphSizes island_sizes(1, 2, 2);          // 1 Feed, 2 Units, 2 Outlets
    std::size_t dual_unit_degrees[] = {2, 2};  // Both Unit 0 and Unit 1 have 2 outputs
    input_node_id isolated_connections[] = {0, 2, 3, 2, 3};  // Unit 1 is completely isolated

    CSRGraph graph_island(island_sizes.n_root_nodes, island_sizes.n_dynamic_nodes,
                          island_sizes.n_sink_nodes,
                          std::span<std::size_t, std::dynamic_extent>(dual_unit_degrees, 2),
                          std::span<input_node_id, std::dynamic_extent>(isolated_connections, 5));

    if (!check_validity(graph_island)) {
        std::cout << "-> SUCCESS: Unreachable isolated unit correctly REJECTED.\n\n";
    } else {
        std::cerr << "-> FATAL ERROR: Broken island topology mistakenly APPROVED!\n";
        return 1;
    }

    // -------------------------------------------------------------------------
    // SUMMARY
    // -------------------------------------------------------------------------
    std::cout << "==================================================\n";
    std::cout << "  ALL VALIDITY CHECKER TESTS PASSED SUCCESSFULLY!  \n";
    std::cout << "==================================================\n";

    return 0;  // Return 0 if all tests pass
}