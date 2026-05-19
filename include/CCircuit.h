/**
  Filename: CCircuit.h
     A class representing a circuit, which is a collection of units and their
 connections.

**/

#pragma once

#include <array>
#include <span>
#include <vector>

#include "CUnit.h"
#include "RequiredFunctions.h"

namespace ESE {
class CSRGraph;
}

bool check_validity(const ESE::CSRGraph& graph);

// Might want to derive from BaseGraph

class Circuit {
  public:
    Circuit();
    explicit Circuit(int num_units);
    explicit Circuit(std::span<const int> circuit_vector);
    bool initialise(std::span<const int> circuit_vector);

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

    static bool check_validity(int vector_size, int*);
    static bool check_validity(int vector_size, int*, int unit_parameters_size,
                               double* unit_parameters);

    double evaluate();

    friend bool check_validity(const ESE::CSRGraph& graph);

  private:
    int num_inputs_ = 0;
    int num_units_ = 0;
    int num_products_ = 0;
    int feed_dest_ = 0;
    std::vector<CUnit> units;
    std::vector<int> empty_outputs_;

    void set_unit_constants(CUnit& unit);
    void mark_units(int unit_num);
    void calculate_all_outputs();
    void save_old_feeds();
    void clear_all_feeds();
    void add_to_unit_feed(int unit_idx, const std::array<double, N_COMPONENTS>& material);
    void add_to_unit_feed(int unit_idx, const double material[N_COMPONENTS]);
    void clear_final_outputs();
    void distribute_outputs();
    bool has_converged() const;

    std::vector<std::array<double, N_COMPONENTS>> final_products;
    std::array<double, N_COMPONENTS> final_tailings{};
};
