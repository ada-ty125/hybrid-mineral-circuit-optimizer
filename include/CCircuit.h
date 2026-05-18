/**
  Filename: CCircuit.h
     A class representing a circuit, which is a collection of units and their connections.

**/

#pragma once



#include <vector>

#include "RequiredFunctions.h"
#include "CUnit.h"

// Might want to derive from BaseGraph

class Circuit {
  public:
    Circuit(int num_units);
    static bool check_validity(int vector_size, int *);
    static bool check_validity(int vector_size, int *,
                               int unit_parameters_size, double *unit_parameters);
  private:
    void mark_units(int unit_num);
    std::vector<CUnit> units;
};

