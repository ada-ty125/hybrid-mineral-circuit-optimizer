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

// Might want to derive from BaseGraph

class Circuit {
 public:
  explicit Circuit(std::span<const int> circuit_vector);

  static bool check_validity(int vector_size, int*);
  static bool check_validity(int vector_size, int*, int unit_parameters_size,
                             double* unit_parameters);

  double evaluate();

 private:
  int feed_dest = 0;
  std::vector<CUnit> units;

  void set_unit_constants(CUnit& unit);
  void mark_units(int unit_num);
  void calculate_all_outputs();
  void save_old_feeds();
  void clear_all_feeds();
  void add_to_unit_feed(int unit_idx,
                        const std::array<double, N_COMPONENTS>& material);
  void add_to_unit_feed(int unit_idx, const double material[N_COMPONENTS]);
  void distribute_outputs();
  bool has_converged() const;
};
