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

/**
 * @brief Total number of chemical/material components tracked in the simulation.
 * * [0] = Palusznium, [1] = Gormanium, [2] = Waste.
 */
constexpr int N_COMPONENTS = 3;

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

    int n_outputs; /**< The total number of downstream output destinations from this specific unit.
                    */

    /**
     * @brief Vector of downstream target IDs.
     * * Maps the output streams to their destination unit IDs or final product IDs.
     */
    std::vector<int> output;

    int unit_type = 0; /**< Technical type classification of the unit: 0 = Type A, 1 = Type B. */

    /** @name Current Iteration Feed States */
    ///@{
    double feed[N_COMPONENTS] = {0.0, 0.0, 0.0}; /**< Current aggregate feed array entering this
                                                    unit. Index map: [0]=P, [1]=G, [2]=W. */
    double feed_P =
        0.0; /**< Convenient individual tracker for current incoming Palusznium mass flow rate. */
    double feed_G =
        0.0; /**< Convenient individual tracker for current incoming Gormanium mass flow rate. */
    double feed_W =
        0.0; /**< Convenient individual tracker for current incoming Waste mass flow rate. */
    ///@}

    /** @name Previous Iteration Feed States (for Convergence Checks) */
    ///@{
    double old_feed[N_COMPONENTS] = {
        0.0, 0.0, 0.0}; /**< Feed mass array recorded from the previous solver iteration. */
    double old_feed_P =
        0.0; /**< Previous iteration tracker for incoming Palusznium mass flow rate. */
    double old_feed_G =
        0.0; /**< Previous iteration tracker for incoming Gormanium mass flow rate. */
    double old_feed_W = 0.0; /**< Previous iteration tracker for incoming Waste mass flow rate. */
    ///@}

    /**
     * @brief Multi-dimensional array tracking the vector flow rates of the concentrate outputs.
     * * Format: `concentrate[i][j]` where:
     * - `i`: Output stream index number.
     * - `j`: Material component index (0 to N_COMPONENTS-1).
     */
    std::vector<std::array<double, N_COMPONENTS>> concentrate;

    double tails[N_COMPONENTS] = {
        0.0, 0.0,
        0.0}; /**< Tailings exit stream vector carrying unrecovered remnants out of the unit. */

    /** @name Graph Traversal & Validity Flags */
    ///@{
    bool mark = false; /**< Visited flag used primarily during recursive graph traversal checks. */
    bool reaches_palusznium = false; /**< Evaluation flag tracking if this cell's output pathway can
                                        reach the final Palusznium stream. */
    bool reaches_gormanium = false;  /**< Evaluation flag tracking if this cell's output pathway can
                                        reach the final Gormanium stream. */
    bool reaches_tailings = false;   /**< Evaluation flag tracking if this cell's output pathway can
                                        reach the final aggregate tailings exit. */
    ///@}

    /** @name Cell Physical Constants & Kinetics */
    ///@{
    double volume = 10.0; /**< Total operational volumetric capacity of the cell tank. */
    double phi = 0.1; /**< Constant multiplier or structural physical coefficient (e.g., gas holdup
                         or fraction parameter). */
    double rho = 3000.0; /**< Volumetric density constant representing solid material components. */
    mutable double residence_time =
        0.0; /**< Calculated fluid stay period within the tank. Marked mutable for lazy-evaluation
                tracking inside const methods. */
    ///@}

    /** @name Simulation & Mathematical Routines */
    ///@{
    /**
     * @brief Computes the absolute sum of all material feeds currently entering this unit.
     * @return Double precision value representing total mass flow (\f$ \text{Feed}_P +
     * \text{Feed}_G + \text{Feed}_W \f$).
     */
    double total_feed() const;

    /**
     * @brief Calculates and updates the internal residence time of the cell based on current
     * throughput metrics.
     * @return The newly computed residence time value.
     */
    double calculate_residence_time() const;

    /**
     * @brief Calculates the recovery rate fraction of a specific mineral component.
     * @param st_idx Stream index target identifier.
     * @param component Targeted mineral column selector index (0 = Palusznium, 1 = Gormanium, 2 =
     * Waste).
     * @param k_matrix Reference array representing kinetic multiplier variables for Type A and Type
     * B units.
     * @return The fractional ratio representing fractional recovery performance (0.0 to 1.0).
     */
    double calculate_recovery(int st_idx, int component, const double k_matrix[2][3]) const;

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