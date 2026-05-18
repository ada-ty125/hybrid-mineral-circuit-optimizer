#include <vector>
#include <stdio.h>

#include "CSRGraph.h"
#include "CUnit.h"
#include "CCircuit.h"

bool check_validity(const CSRGraph& graph) {
  // This is function that checks if the graph is valid

  // Current dummy version just returns true.

  return true;
}

Circuit::Circuit(int num_units) {
  units.resize(num_units);

  // potentially set up units further
}

void Circuit::mark_units(int unit_num) {

  if (units[unit_num].mark) return;

  units[unit_num].mark = true;

  //If we have seen this unit already exit
  //Mark that we have now seen the unit

  for (int i=0; i<units[unit_num].n_outputs; i++) {
    //If output_unit_index does not point at a circuit outlet recursively call the function
    if (units[unit_num].output[i]<units.size()) {
      mark_units(units[unit_num].output[i]);
    } else {
      // ...Potentially do something to indicate that you have seen an exit
    }
  }

}


