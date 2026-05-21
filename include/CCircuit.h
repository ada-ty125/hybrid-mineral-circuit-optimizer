/**
 * @file CCircuit.h
 * @brief Defines the Circuit structure and its simulation parameters for mineral processing
 * networks.
 * @author Cuprite Team (ACDS Palusznium Rush)
 * @date 2026-05-21
 * * This file contains the declaration of the Circuit class and the Simulator_Parameters struct.
 * It manages the topology, initialization, and reachability analysis of a mineral separation
 * circuit composed of multiple CUnit entities connected in a directed graph layout.
 */

#pragma once

#include <array>
#include <span>
#include <vector>
#include <string>

/**
 * @struct Simulator_Parameters
 * @brief Holds configuration parameters, solver mechanics tolerances, and kinetic matrices for the
 * simulation.
 */
struct Simulator_Parameters {
    // Solver Mechanics
    double tolerance = 1e-6;
    int max_iterations = 10000;
    double min_denominator = 1e-12;
    // physical properties
    double tank_volume = 10.0;
    double fluid_density = 3000.0;
    double phi = 0.1;
    // Components
    int n_components = 3;
    std::vector<std::string> component_names = {"Palusznium", "Gormanium", "Waste"};
    // A single vector handles all component feed inputs
    std::vector<double> input_feed_rates = {8.0, 12.0, 80.0};
    // k matrix values based on Type
    std::vector<std::vector<double>> k_TypeA = {{0.008, 0.006, 0.0005}, {0.0, 0.0, 0.0}};
    std::vector<std::vector<double>> k_TypeB = {{0.007, 0.001, 0.001}, {0.001, 0.006, 0.001}};
};

/**
 * @brief Global instantiation of default simulator constants.
 */
extern Simulator_Parameters default_simulator_parameters;

#include "CUnit.h"
#include "RequiredFunctions.h"

namespace ESE {
class CSRGraph;
}

class CSimulator;

/**
 * @brief Global validity check function operating on an ESE Compressed Sparse Row (CSR) graph
 * representation.
 * @param graph A reference to the CSRGraph topology.
 * @return True if the graph satisfies structural integrity constraints, false otherwise.
 */
bool check_validity(const ESE::CSRGraph& graph);

/**
 * @class Circuit
 * @brief Represents a comprehensive processing circuit network, handling topological verification
 * and material distribution.
 */
class Circuit {
  public:
    /** @name Constructors & Destructors */
    ///@{
    /**
     * @brief Construct a new, empty Circuit object.
     */
    Circuit();

    /**
     * @brief Construct a Circuit object with a predefined number of unlinked units.
     * @param num_units Total number of separation units to allocate.
     */
    explicit Circuit(int num_units);

    /**
     * @brief Construct and automatically initialize a Circuit from a vector encoding.
     * @param circuit_vector Flattened list representing circuit vector routing.
     */
    explicit Circuit(std::span<const int> circuit_vector);
    ///@}

    /**
     * @brief Initializes the circuit layout and parameters.
     * @param circuit_vector Structure routing vector defining the directed connections.
     * @param simulator_parameters Simulation parameters (defaults to default_simulator_parameters).
     * @return True if successfully initialized and valid, false otherwise.
     */
    bool initialise(
        std::span<const int> circuit_vector,
        const Simulator_Parameters& simulator_parameters = default_simulator_parameters);

    /** @name Component Queries */
    ///@{
    /**
     * @brief Checks if a given integer corresponds to a valid unit ID.
     */
    bool is_unit_id(int id) const noexcept;

    /**
     * @brief Checks if a given integer corresponds to a final product ID destination.
     */
    bool is_product_id(int id) const noexcept;

    /**
     * @brief Translates a global product destination ID into its relative matrix index tracker.
     */
    int product_index(int product_id) const noexcept;
    ///@}

    /** @name Topology & Reachability Analysis */
    ///@{
    /**
     * @brief Determines which units can be reached starting from the initial feed point.
     * @return A boolean vector where index `i` is true if unit `i` is accessible.
     */
    std::vector<bool> units_reachable_from_feed() const;

    /**
     * @brief Evaluates path connectivity from a start unit to a target node.
     */
    bool can_reach(int start, int target) const;

    /**
     * @brief Gathers final product destinations reachable from a specific unit.
     */
    std::vector<bool> products_reachable_from_unit(int unit_id) const;

    /**
     * @brief Counts the total amount of unique product nodes accessible from a given unit.
     */
    int count_reachable_products_from_unit(int unit_id) const;
    ///@}

    /** @name Getters */
    ///@{
    int num_inputs() const noexcept;   /**< Returns the count of input feeds. */
    int num_units() const noexcept;    /**< Returns total separation units currently present. */
    int num_products() const noexcept; /**< Returns total available exit product routes. */
    int feed_dest() const noexcept;    /**< Returns the entry unit index ID for the feed. */

    /**
     * @brief Retrieves the assigned downstream routing paths for a target unit.
     */
    const std::vector<int>& output_destinations(int unit_id) const;
    ///@}

    /** @name Static Validity Checkers */
    ///@{
    /**
     * @brief Static validator assessing a raw integer vector structure layout.
     */
    static bool check_validity(int vector_size, int* circuit_vector);

    /**
     * @brief Static validator assessing both vector routing layout and specific mechanical
     * parameter bounds.
     */
    static bool check_validity(int vector_size, int* circuit_vector, int unit_parameters_size,
                               double* unit_parameters);
    ///@}

    friend bool check_validity(const ESE::CSRGraph& graph);
    friend class CSimulator;

    int num_inputs_ = 0;
    int num_units_ = 0;
    int num_products_ = 0;
    int feed_dest_ = 0;

    std::vector<CUnit> units;
    std::vector<int> empty_outputs_;

    std::vector<std::vector<double>> final_products;
    std::vector<double> final_tailings;

    void set_unit_constants(CUnit& unit, const Simulator_Parameters& params);

    /**
     * @brief Performs internal recursive graph traversal flag marking for accessibility audits.
     */
    void mark_units(int unit_num);
};