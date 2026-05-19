/** Header for the unit class
 *
 *
 */

#pragma once

#include <array>
#include <vector>

constexpr int N_COMPONENTS = 3;

class CUnit {
  public:
    int n_outputs;  // The number of outputs from this unit

    // Output from this unit where [0] = Palusznium, [1] = Gormanium, [2] = Waste
    std::vector<int> output;

    // Current feed entering this unit
    // [0] = Palusznium
    // [1] = Gormanium
    // [2] = Waste
    double feed[N_COMPONENTS] = {0.0, 0.0, 0.0};

    // Feed from previous iteration
    double old_feed[N_COMPONENTS] = {0.0, 0.0, 0.0};

    // Concentrate output streams
    // concentrate[i][j]
    // i = output stream number
    // j = component
    std::vector<std::array<double, N_COMPONENTS>> concentrate;

    // Tailings stream
    double tails[N_COMPONENTS] = {0.0, 0.0, 0.0};

    // Used for validity checking
    bool mark = false;

    // Constants for calculations
    double volume = 10.0;
    double phi = 0.1;
    double rho = 3000.0;

    // Recovery constants
    // k[0] = Palusznium
    // k[1] = Gormanium
    // k[2] = Waste
    double k[N_COMPONENTS] = {0.0, 0.0, 0.0};

    // Stream 1 and 2 constants (zero for TypeA Stream 2)
    double k_matrix[2][N_COMPONENTS] = {{0.008, 0.006, 0.0005}, {0.0, 0.0, 0.0}};

    // Simulation functions
    double total_feed() const;
    double calculate_residence_time() const;

    double calculate_recovery(int st_idx, int component) const;

    void calculate_outputs();

    void clear_feeds();
};
