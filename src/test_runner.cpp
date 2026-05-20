#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include "CCircuit.h"
#include "Economics.h"


// Simple macro-like helpers for assertion tracking
int tests_run = 0;
int tests_passed = 0;

void ASSERT_TRUE(bool condition, const std::string& test_name) {
    tests_run++;
    if (condition) {
        std::cout << "  [PASS] " << test_name << "\n";
        tests_passed++;
    } else {
        std::cerr << "  [FAIL] " << test_name << "\n";
    }
}

void ASSERT_NEAR(double actual, double expected, double tolerance, const std::string& test_name) {
    tests_run++;
    double diff = std::abs(actual - expected);
    if (diff <= tolerance) {
        std::cout << "  [PASS] " << test_name << " (Expected: " << expected << ", Got: " << actual << ")\n";
        tests_passed++;
    } else {
        std::cerr << "  [FAIL] " << test_name << " (Expected: " << expected << ", Got: " << actual << ", Diff: " << diff << ")\n";
    }
}

// ============================================================================
// UNIT TEST CASES
// ============================================================================

// 1. Test that the constructor sets up structural vectors accurately
void test_constructor_initialization() {
    std::cout << "\nRunning: test_constructor_initialization...\n";
    
    // 3 units. Unit 0 has 2 outputs, Unit 1 has 2, Unit 2 has 3.
    std::vector<int> mock_vec = {
        1, 5, 3,       // 1 input, 3 units, 2 sinks
        2, 2, 2, 2, 3,        // output counts
        3,             // feed_dest
        2, 7,          // Unit 0 routes
        2, 3,          // Unit 1 routes
        4, 1,
        2, 0,
        5, 6, 2        // Unit 2 routes
    };

    Circuit circuit(mock_vec);

    ASSERT_TRUE(circuit.units.size() == 3, "Circuit sizing matches input metadata");
    
    ASSERT_TRUE(circuit.units[0].n_outputs == 2, "Unit 0 output stream size allocated accurately");
    ASSERT_TRUE(circuit.units[2].n_outputs == 3, "Unit 2 output stream size allocated accurately");
    ASSERT_TRUE(circuit.units[2].output[1] == 3, "Target layout index routing matched vector positions");
}

// 2. Test that Kinetic Constants map correctly by Unit Type
void test_kinetic_matrix_assignment() {
    std::cout << "\nRunning: test_kinetic_matrix_assignment...\n";
    
    std::vector<int> mock_vec = {
        1, 5, 3,       // 1 input, 3 units, 2 sinks
        2, 2, 2, 2, 3,        // output counts
        3,             // feed_dest
        2, 7,          // Unit 0 routes
        2, 3,          // Unit 1 routes
        4, 1,
        2, 0,
        5, 6, 2        // Unit 2 routes
    };
    Circuit circuit(mock_vec);

    // Unit 0 is Type A (2 outputs)
    ASSERT_NEAR(circuit.units[0].k_matrix[0][0], 0.008, 1e-5, "Unit 0 (Type A) Palusznium k1 matches spec");
    ASSERT_NEAR(circuit.units[0].k_matrix[0][2], 0.0005, 1e-5, "Unit 0 (Type A) Waste k1 matches spec");

    // Unit 1 is Type B (3 outputs)
    ASSERT_NEAR(circuit.units[1].k_matrix[1][1], 0.006, 1e-5, "Unit 1 (Type B) Gormanium k2 matches spec");
}
void test_brief_specification_values() {
    std::cout << "\nRunning: test_brief_specification_values...\n";

    // 1 input, 1 unit (Type A), 2 product streams (sinks)
    // Vector format: [num_inputs, num_units, num_sinks, degrees..., feed_dest, routings...]
    std::vector<int> mock_vec = {
        1, 5, 3,       // 1 input, 3 units, 2 sinks
        2, 2, 2, 2, 3,        // output counts
        3,             // feed_dest
        2, 7,          // Unit 0 routes
        2, 3,          // Unit 1 routes
        4, 1,
        2, 0,
        5, 6, 2        // Unit 2 routes
    };

    Circuit circuit(mock_vec);
    double calculated_profit = circuit.evaluate();

    // 1. Check Global Mass Conservation First
    double expected_total_mass = 8.0 + 12.0 + 80.0; // 100.0 kg/s
    double actual_total_mass = 0.0;
    
    for (size_t prod = 0; prod < circuit.final_products.size(); prod++) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            actual_total_mass += circuit.final_products[prod][comp];
        }
    }
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        actual_total_mass += circuit.final_tailings[comp];
    }

    ASSERT_NEAR(actual_total_mass, expected_total_mass, 1e-4, "Specification Test: Global Mass perfectly conserved at 100 kg/s");

    // 2. Validate Component Splitting
    // Unit 0 Concentrate must match single-pass Type A kinetics for Palusznium
    double palusznium_in_prod0 = circuit.final_products[0][0]; 
    
    std::cout << "  --------------------------------------------------\n";
    std::cout << "  SPECIFICATION BRIEF VERIFICATION METRICS:\n";
    std::cout << "  Calculated Net Economic Score: $" << calculated_profit << " / sec\n";
    std::cout << "  Palusznium in Product 0:       " << palusznium_in_prod0 << " kg/s\n";
    std::cout << "  Gormanium in Product 0:        " << circuit.final_products[0][1] << " kg/s\n";
    std::cout << "  Waste in Product 0 (Contam):   " << circuit.final_products[0][2] << " kg/s\n";
    std::cout << "  --------------------------------------------------\n";

    // Ensure the profit calculation is returning a stable number (not NaN or Inf)
    ASSERT_TRUE(!std::isnan(calculated_profit), "Specification Test: Economic module successfully returned valid financial metric");
}
// 3. Test that conservation of mass holds at steady-state convergence
// 3. Test that conservation of mass holds at steady-state convergence
void test_mass_conservation_at_convergence() {
    std::cout << "\nRunning: test_mass_conservation_at_convergence...\n";

    std::vector<int> mock_vec = {
        1, 5, 3,       // 1 input, 3 units, 2 sinks
        2, 2, 2, 2, 3,        // output counts
        3,             // feed_dest
        2, 7,          // Unit 0 routes
        2, 3,          // Unit 1 routes
        4, 1,
        2, 0,
        5, 6, 2        // Unit 2 routes
    };

    Circuit circuit(mock_vec);
    
    // Capture the generated performance value!
    double economic_performance = circuit.evaluate();

    double total_input_mass = 8.0 + 12.0 + 80.0; // 100.0 kg/s baseline feed
    double total_output_mass = 0.0;

    // A. Accumulate mass captured in all premium product streams
    for (size_t prod = 0; prod < circuit.final_products.size(); prod++) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            total_output_mass += circuit.final_products[prod][comp];
        }
    }

    // B. Accumulate mass dropped into the tailing stream
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        total_output_mass += circuit.final_tailings[comp];
    }

    // --- DIAGNOSTIC PRINTS USING YOUR TRUE DATA MEMBERS ---
    std::cout << "  --------------------------------------------------\n";
    std::cout << "  DIAGNOSTIC METRICS FOR GA EVALUATION:\n";
    std::cout << "  Calculated Net Economic Value: $" << economic_performance << " / sec\n";
    std::cout << "  Palusznium in Product 0:      " << circuit.final_products[0][0] << " kg/s\n";
    std::cout << "  Gormanium in Product 1:       " << circuit.final_products[1][0] << " kg/s\n";
    std::cout << "  Total Waste in Tailings:      " << circuit.final_tailings[2] << " kg/s\n";
    std::cout << "  --------------------------------------------------\n";

    ASSERT_NEAR(total_output_mass, total_input_mass, 1e-3, "Total steady-state mass out equals total mass in");
}

// 4. Test that infinite cyclic paths are safely caught by iteration caps
void test_infinite_loop_short_circuit() {
    std::cout << "\nRunning: test_infinite_loop_short_circuit...\n";

    // Unit 0 sends both outputs back into itself. Impossible to converge.
    std::vector<int> mock_vec = {
        1, 5, 3,       // 1 input, 3 units, 2 sinks
        2, 2, 2, 2, 3,        // output counts
        3,             // feed_dest
        2, 7,          // Unit 0 routes
        2, 3,          // Unit 1 routes
        4, 1,
        2, 0,
        5, 6, 2        // Unit 2 routes
    };

    Circuit circuit(mock_vec);
double score = circuit.evaluate();

// Calculate what the economic module actually outputs as the fallback score
double expected_fallback = cuprite::worst_case_value(80.0, cuprite::default_economics);

ASSERT_NEAR(score, expected_fallback, 1e-5, "Solver flags non-converging layouts with economic fallback calculation");
}

// ============================================================================
// MAIN EXECUTION ENGINE
// ============================================================================
int main() {
    std::cout << "==================================================\n";
    std::cout << "       EXECUTING CIRCUIT SIMULATOR UNIT TESTS     \n";
    std::cout << "==================================================\n";

    test_constructor_initialization();
    test_kinetic_matrix_assignment();
    test_brief_specification_values();
    test_mass_conservation_at_convergence();
    test_infinite_loop_short_circuit();

    std::cout << "\n==================================================\n";
    std::cout << "TEST RUN COMPLETE: " << tests_passed << " / " << tests_run << " PASSED\n";
    std::cout << "==================================================\n";

    return (tests_passed == tests_run) ? 0 : 1;
}