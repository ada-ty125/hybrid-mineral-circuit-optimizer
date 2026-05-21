/**
  Filename: CCircuit.h
     A class representing a circuit, which is a collection of units and their
 connections.

**/

#pragma once

#include <array>
#include <span>
#include <vector>
#include <string>

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
extern Simulator_Parameters default_simulator_parameters;

#include "CUnit.h"
#include "RequiredFunctions.h"

namespace ESE {
class CSRGraph;
}

class CSimulator;

bool check_validity(const ESE::CSRGraph& graph);

class Circuit {
  public:
    Circuit();
    explicit Circuit(int num_units);
    explicit Circuit(std::span<const int> circuit_vector);

    bool initialise(
        std::span<const int> circuit_vector,
        const Simulator_Parameters& simulator_parameters = default_simulator_parameters);

    bool is_unit_id(int id) const noexcept;
    bool is_product_id(int id) const noexcept;
    int product_index(int product_id) const noexcept;

    std::vector<bool> units_reachable_from_feed() const;
    bool can_reach(int start, int target) const;
    std::vector<bool> products_reachable_from_unit(int unit_id) const;
    int count_reachable_products_from_unit(int unit_id) const;

    int num_inputs() const noexcept;
    int num_units() const noexcept;
    int num_products() const noexcept;
    int feed_dest() const noexcept;
    const std::vector<int>& output_destinations(int unit_id) const;

    static bool check_validity(int vector_size, int* circuit_vector);
    static bool check_validity(int vector_size, int* circuit_vector, int unit_parameters_size,
                               double* unit_parameters);

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
    void mark_units(int unit_num);
};
