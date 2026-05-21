/** Header for the unit class
 *
 *
 */

#pragma once

#include <array>
#include <vector>

struct Simulator_Parameters;

class CUnit {
  public:
    CUnit();

    int n_outputs;
    int unit_type;

    std::vector<int> output;
    std::vector<double> feed;
    std::vector<double> old_feed;
    std::vector<std::vector<double>> concentrate;
    std::vector<double> tails;
    std::vector<double> total_recovery;
    bool mark = false;
    std::vector<std::vector<double>> k_matrix;

    // Constants for calculations
    mutable double residence_time = 0.0;

    // Simulation functions
    double total_feed() const;
    double calculate_residence_time(const Simulator_Parameters& params) const;

    double calculate_recovery(int st_idx, int component,
                              const std::vector<std::vector<double>>& k_matrix, double ta) const;

    void calculate_outputs(const Simulator_Parameters& params);

    void clear_feeds();
};
