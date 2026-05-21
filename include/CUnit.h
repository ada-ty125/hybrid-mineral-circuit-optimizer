/**
 * @file CUnit.h
 * @brief Defines the CUnit class representing an individual separation unit in a mineral processing
 * circuit.
 * @author Cuprite Team (ACDS Palusznium Rush)
 * @date 2026-05-21
 * * This file contains the CUnit class declaration, which encapsulates the physical properties,
 * feed states, kinetic parameters, and stream calculations (concentrate and tailings) for a single
 * cell inside the simulation network.
 */

#pragma once

#include <array>
#include <vector>

struct Simulator_Parameters;

/**
 * @class CUnit
 * @brief Represents a single separation cell/unit within the mineral processing flowsheet.
 * * Each unit processes an incoming aggregate feed stream and splits it into multiple
 * downstream output destinations (concentrate streams and a tailings stream) based on its
 * residence time and recovery physics.
 */
class CUnit {
  public:
    /**
     * @brief Construct a new CUnit object with default initial states.
     */
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

    /** @name Simulation & Mathematical Routines */
    ///@{
    /**
     * @brief Computes the absolute sum of all material feeds currently entering this unit.
     * @return Double precision value representing total mass flow (\f$ \text{Feed}_P +
     * \text{Feed}_G + \text{Feed}_W \f$).
     */
    double total_feed() const;
    double calculate_residence_time(const Simulator_Parameters& params) const;

    double calculate_recovery(int st_idx, int component,
                              const std::vector<std::vector<double>>& k_matrix, double ta) const;

    /**
     * @brief Executes the simulation separation laws to compute and distribute current material
     * into concentrate and tailings structures.
     * @param params Global parameter settings struct storing constraints and kinetic profiles.
     */
    void calculate_outputs(const Simulator_Parameters& params);

    /**
     * @brief Resets all current and historical feed trackers back to zero.
     * * Typically invoked to clean up unit states before running a fresh iterative cycle.
     */
    void clear_feeds();
    ///@}
};