#include "CUnit.h"
#include "CCircuit.h"

#include <cmath>

CUnit::CUnit() {
    n_outputs = 2;
    unit_type = 0;
    output.resize(2, 0);
}

/**
 * Resets the current incoming mass feed components to zero.
 */
void CUnit::clear_feeds() { std::fill(feed.begin(), feed.end(), 0.0); }

/**
 * Sums the mass flow rates of all individual fluid components.
 * @return Total mass flow entering the unit (kg/s).
 */
double CUnit::total_feed() const {
    double total_feed = 0.0;
    for (double component_flow : feed) {
        total_feed += component_flow;
    }
    return total_feed;
}

/**
 * Calculates the fluid residence time (tau) within the separation tank.
 * Uses the relation: tau = (phi * V) / Q, where Q is volumetric flow rate.
 * @return Residence time in seconds.
 */
double CUnit::calculate_residence_time(const Simulator_Parameters& params) const {
    double total_mass_flow = total_feed();

    // Prevent division by zero if the tank has no incoming feed
    if (total_mass_flow < params.min_denominator) {
        total_mass_flow = params.min_denominator;
    }
    double total_vol_flow = total_mass_flow / params.fluid_density;
    residence_time = (params.phi * params.tank_volume) / total_vol_flow;
    return residence_time;
}

/**
 * Computes fractional recovery (R) for a component using a multi-stream
 * competitive kinetic framework: R = (k_i * tau) / (1 + sum(k_j * tau)).
 * @param st_idx Index of the target output concentrate stream.
 * @param component Index of the chemical component (Pal, Gor, Waste).
 * @return Fractional recovery value between 0.0 and 1.0.
 */
double CUnit::calculate_recovery(int st_idx, int component,
                                 const std::vector<std::vector<double>>& k_matrix,
                                 double tau) const {
    double summation = 0.0;
    int num_c_stream = n_outputs - 1;

    // Sums kinetic factors across all available concentrate streams
    for (int m = 0; m < num_c_stream; m++) {
        summation += k_matrix[m][component] * tau;
    }
    double num = k_matrix[st_idx][component] * tau;

    return num / (1.0 + summation);
}

/**
 * Simulates unit separation physics. Computes output mass flows for all
 * concentrate streams and applies mass conservation to calculate the tailings.
 */
void CUnit::calculate_outputs(const Simulator_Parameters& params) {
    int n_comp = params.n_components;
    int num_c_streams = n_outputs - 1;
    concentrate.resize(num_c_streams, std::vector<double>(n_comp, 0.0));
    tails.resize(n_comp, 0.0);

    double total_incoming_feed = total_feed();

    if (total_incoming_feed < params.min_denominator) {
        for (int out = 0; out < num_c_streams; out++) {
            std::fill(concentrate[out].begin(), concentrate[out].end(), 0.0);
        }
        std::fill(tails.begin(), tails.end(), 0.0);
        return;
    }

    double tau = calculate_residence_time(params);

    const auto& k_matrix = (unit_type == 0) ? params.k_TypeA : params.k_TypeB;
    std::vector<double> total_recovery(n_comp, 0.0);

    // Calculating mass partitioned into each concentrate stream
    for (int out = 0; out < num_c_streams; out++) {
        for (int comp = 0; comp < n_comp; comp++) {
            double R = calculate_recovery(out, comp, k_matrix, tau);
            concentrate[out][comp] = feed[comp] * R;
            total_recovery[comp] += R;
        }
    }
    // Tailings mass is the remainder after accounting for all recoveries.
    for (int comp = 0; comp < n_comp; comp++) {
        tails[comp] = feed[comp] * (1.0 - total_recovery[comp]);
    }
}