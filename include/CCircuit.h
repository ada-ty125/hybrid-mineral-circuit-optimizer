/**
 * Filename: CCircuit.h
 * A class representing a circuit, which is a collection of units and their connections.
 **/

#pragma once

#include <span>
#include <vector>

#include "RequiredFunctions.h"
#include "CUnit.h"
#include "CSRGraph.h"  // Needed for the global check_validity interface

// Forward declaration of the global validation function to ensure friendship binds correctly
namespace ESE {
class CSRGraph;
}
bool check_validity(const ESE::CSRGraph& graph);

class Circuit {
  public:
    Circuit();
    Circuit(int num_units);
    bool initialise(std::span<const int> circuit_span);

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
    static bool check_validity(int vector_sßize, int*, int unit_parameters_size,
                               double* unit_parameters);

    // Give friendship to the high-performance check_validity function
    // so it can seamlessly populate structural boundary fields (STEP 2 optimization)
    friend bool check_validity(const ESE::CSRGraph& graph);

  private:
    void mark_units(int unit_num);

    int num_inputs_;
    int num_units_;
    int num_products_;
    int feed_dest_;
    std::vector<int> output_counts_;
    std::vector<int> empty_outputs_;
    std::vector<CUnit> units;
};