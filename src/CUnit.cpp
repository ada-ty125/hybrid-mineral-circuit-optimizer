#include "CUnit.h"

#include <cmath>

CUnit::CUnit() {
    n_outputs = 2;
    unit_type = 0;
    output.resize(2, 0);
}

/**
 * Resets the current incoming mass feed components to zero.
 */
void CUnit::clear_feeds() {
    for (int i = 0; i < N_COMPONENTS; i++) {
        feed[i] = 0.0;
    }
    feed_P = 0.0;
    feed_G = 0.0;
    feed_W = 0.0;
}

/**
 * Sums the mass flow rates of all individual fluid components.
 * @return Total mass flow entering the unit (kg/s).
 */
double CUnit::total_feed() const {
    double total_feed = 0.0;
    for (int i = 0; i < N_COMPONENTS; i++) {
        total_feed += feed[i];
    }
    return total_feed;
}

/**
 * Calculates the fluid residence time (tau) within the separation tank.
 * Uses the relation: tau = (phi * V) / Q, where Q is volumetric flow rate.
 * @return Residence time in seconds.
 */
double CUnit::calculate_residence_time() const {
    double total_mass_flow = total_feed();

    // Prevent division by zero if the tank has no incoming feed
    if (total_mass_flow < 1e-10) {
        total_mass_flow = 1e-10;
    }
    double total_vol_flow = total_mass_flow / rho;
    residence_time = (phi * volume) / total_vol_flow;
    return residence_time;
}

/**
 * Computes fractional recovery (R) for a component using a multi-stream
 * competitive kinetic framework: R = (k_i * tau) / (1 + sum(k_j * tau)).
 * @param st_idx Index of the target output concentrate stream.
 * @param component Index of the chemical component (Pal, Gor, Waste).
 * @return Fractional recovery value between 0.0 and 1.0.
 */
double CUnit::calculate_recovery(int st_idx, int component) const {
    double tau = calculate_residence_time();
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
void CUnit::calculate_outputs() {
    int num_c_streams = n_outputs - 1;
    concentrate.resize(num_c_streams);
    double total_recovery[N_COMPONENTS] = {0.0, 0.0, 0.0};

    // Calculating mass partitioned into each concentrate stream
    for (int out = 0; out < num_c_streams; out++) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            double R = calculate_recovery(out, comp);
            concentrate[out][comp] = feed[comp] * R;
            total_recovery[comp] += R;
        }
    }
    // Tailings mass is the remainder after accounting for all recoveries.
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        tails[comp] = feed[comp] * (1.0 - total_recovery[comp]);
    }
}