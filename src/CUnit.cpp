/**
 * @file CUnit.cpp
 * @brief Implements the mathematical models, kinetic frameworks, and physical separation calculations for a single CUnit.
 * @author Cuprite Team (ACDS Palusznium Rush)
 * @date 2026-05-21
 */

#include "CUnit.h"
#include "CCircuit.h"

#include <cmath>

/**
 * @brief Construct a new CUnit::CUnit object.
 * Initializes default properties for a Type A cell with 2 output ports.
 */
CUnit::CUnit() {
    n_outputs = 2;
    unit_type = 0;
    output.resize(2, 0);
}

void CUnit::clear_feeds() {
    for (int i = 0; i < N_COMPONENTS; i++) {
        feed[i] = 0.0;
    }
    feed_P = 0.0;
    feed_G = 0.0;
    feed_W = 0.0;
}

double CUnit::total_feed() const {
    double total_feed = 0.0;
    for (int i = 0; i < N_COMPONENTS; i++) {
        total_feed += feed[i];
    }
    return total_feed;
}

/**
 * @details Computes the residence time using the relation:
 * \f[
 * \tau = \frac{\phi \cdot V}{Q} = \frac{\phi \cdot V \cdot \rho}{\dot{m}_{\text{total}}}
 * \f]
 * Where:
 * - \f$ V \f$ is the cell operational volume.
 * - \f$ \phi \f$ is the physical coefficient/fractional constant.
 * - \f$ \rho \f$ is the material density.
 * - \f$ \dot{m}_{\text{total}} \f$ is the total mass flow rate.
 * * Includes a lower-bound threshold check (\f$ 1.0 \times 10^{-10} \f$) to avoid division-by-zero scenarios when no feed enters the tank.
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
 * @details Implements a multi-stream competitive first-order flotation/separation kinetic equation:
 * \f[
 * R_{i,c} = \frac{k_{i,c} \cdot \tau}{1 + \sum_{m=0}^{N_{c}-1} k_{m,c} \cdot \tau}
 * \f]
 * Where:
 * - \f$ R_{i,c} \f$ is the fractional recovery of component \f$ c \f$ into concentrate stream \f$ i \f$.
 * - \f$ k_{i,c} \f$ is the specific kinetic rate constant for component \f$ c \f$ in stream \f$ i \f$.
 * - \f$ \tau \f$ is the dynamic fluid residence time within the unit.
 * - \f$ N_{c} \f$ is the total number of available concentrate output paths (\f$ N_{\text{outputs}} - 1 \f$).
 */
double CUnit::calculate_recovery(int st_idx, int component, const double k_matrix[2][3]) const {
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
 * @details Executes full mass-balance partitioning across the cell block.
 * * 1. Resizes the multi-dimensional `concentrate` array according to the calculated fraction of concentrate lines.
 * 2. Fetches the recovery metrics \f$ R \f$ corresponding to the respective `unit_type` matrix specifications (Type A vs Type B).
 * 3. Distributes the component mass into individual concentrate stream indices:
 * \f[ \dot{m}_{\text{conc}, i, c} = \dot{m}_{\text{feed}, c} \cdot R_{i,c} \f]
 * 4. Enforces the conservation of mass law to group the residual unrecovered elements into the tailings stream:
 * \f[ \dot{m}_{\text{tails}, c} = \dot{m}_{\text{feed}, c} \cdot \left(1.0 - \sum_{i} R_{i,c}\right) \f]
 */
void CUnit::calculate_outputs(const Simulator_Parameters& params) {
    int num_c_streams = n_outputs - 1;
    concentrate.resize(num_c_streams);
    double total_recovery[N_COMPONENTS] = {0.0, 0.0, 0.0};

    // Calculating mass partitioned into each concentrate stream
    for (int out = 0; out < num_c_streams; out++) {
        for (int comp = 0; comp < N_COMPONENTS; comp++) {
            double R = unit_type == 0 ? calculate_recovery(out, comp, params.k_TypeA)
                                      : calculate_recovery(out, comp, params.k_TypeB);
            concentrate[out][comp] = feed[comp] * R;
            total_recovery[comp] += R;
        }
    }
    // Tailings mass is the remainder after accounting for all recoveries.
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
        tails[comp] = feed[comp] * (1.0 - total_recovery[comp]);
    }
}