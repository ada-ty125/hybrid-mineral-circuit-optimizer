#include <vector>
#include <algorithm>
#include <stdio.h>

#include "CSRGraph.h"
#include "CUnit.h"
#include "CCircuit.h"


/**
 * @brief High-performance implementation of circuit validity checking.
 * @param graph The compressed sparse row graph representing the circuit topology.
 * @return true if the circuit meets all 4 golden standards, false otherwise.
 */
bool check_validity(const ESE::CSRGraph& graph) {
    // -------------------------------------------------------------------------
    // STEP 1: Ultra-Fast Local Checks (Student B's Territory)
    // Intercept invalid configurations instantly without any graph traversal.
    // -------------------------------------------------------------------------
    // In this framework, n_dynamic_nodes represents the total number of Separation Units
    int num_units = static_cast<int>(graph.n_dynamic_nodes);
    if (num_units <= 0) return false;

    for (int i = 0; i < num_units; ++i) {
        // get_degree(i) returns the number of outgoing streams for unit i
        int n_outputs = static_cast<int>(graph.get_degree(i));
        if (n_outputs <= 0) return false; // Basic safety bounds
        
        // node_targets(i, j) fetches the destination of the j-th output from unit i
        int first_dest = static_cast<int>(graph.node_targets(i, 0));
        bool all_outputs_identical = true;

        for (int j = 0; j < n_outputs; ++j) {
            int dest = static_cast<int>(graph.node_targets(i, j));
            
            // Condition 3: Self-recycle is strictly prohibited
            if (dest == i) {
                return false; 
            }
            if (dest != first_dest) {
                all_outputs_identical = false;
            }
        }

        // Condition 4: Outlets from a single unit must not merge into the exact same node
        if (n_outputs > 1 && all_outputs_identical) {
            return false; 
        }
    }

    // -------------------------------------------------------------------------
    // STEP 2: Instantiating and Parsing the Circuit (Student A's Territory)
    // Synchronize external graph data into our tailored Circuit object.
    // -------------------------------------------------------------------------
    Circuit circuit(num_units);
    
    // According to standard parsing: the root node (Feed) connects to the starting unit.
    // We fetch the first target of the circuit feed boundary.
    int feed_destination = 0; 
    if (graph.input_index != nullptr) {
        feed_destination = static_cast<int>(graph.input_index[0]);
    } else {
        return false; // Safety fallback if graph is uninitialized
    }

    circuit.num_inputs_ = 1;      // Standard benchmark baseline
    circuit.num_products_ = 3;    // Palusznium, Gormanium, Final Tailings
    circuit.feed_dest_ = feed_destination;

    for (int i = 0; i < num_units; ++i) {
        circuit.units[i].n_outputs = static_cast<int>(graph.get_degree(i));
        circuit.units[i].unit_type = (circuit.units[i].n_outputs == 2) ? 0 : 1;
        circuit.units[i].output.resize(static_cast<std::size_t>(circuit.units[i].n_outputs));
        
        for (int j = 0; j < circuit.units[i].n_outputs; ++j) {
            circuit.units[i].output[static_cast<std::size_t>(j)] = static_cast<int>(graph.node_targets(i, j));
        }
    }

    // -------------------------------------------------------------------------
    // STEP 3: Condition 1 Verification (Accessible from Feed)
    // Use your existing stack-driven 'units_reachable_from_feed' function.
    // -------------------------------------------------------------------------
    std::vector<bool> reachable_from_feed = circuit.units_reachable_from_feed();
    for (int i = 0; i < num_units; ++i) {
        if (!reachable_from_feed[static_cast<std::size_t>(i)]) {
            return false; // Found an unreachable unit island!
        }
    }

    // -------------------------------------------------------------------------
    // STEP 4: Condition 2 Verification (Routes to at least 2 different outlets)
    // Use your existing stack-driven 'count_reachable_products_from_unit' function.
    // -------------------------------------------------------------------------
    for (int i = 0; i < num_units; ++i) {
        int reachable_products_count = circuit.count_reachable_products_from_unit(i);
        if (reachable_products_count < 2) {
            return false; // Material trapped or insufficient separation streams!
        }
    }

    // If it survives all checks, the circuit topology is safe and valid!
    return true;
}

Circuit::Circuit()
    : num_inputs_(0),
      num_units_(0),
      num_products_(0),
      feed_dest_(-1) {}

Circuit::Circuit(int num_units)
    : num_inputs_(0),
      num_units_(std::max(num_units, 0)),
      num_products_(0),
      feed_dest_(-1),
      output_counts_(static_cast<std::size_t>(std::max(num_units, 0)), 0) {
  if (num_units_ > 0) {
    units.resize(static_cast<std::size_t>(num_units_));
  }

  // Legacy constructor: allocate unit storage, but leave parsed header fields unset.
}

bool Circuit::initialise(std::span<const int> circuit_span) {
  if (circuit_span.size() < 4) {
    return false;
  }

  const int parsed_num_inputs = circuit_span[0];
  const int parsed_num_units = circuit_span[1];
  const int parsed_num_products = circuit_span[2];

  if (parsed_num_units <= 0) {
    return false;
  }

  // Layout: [num_inputs, num_units, num_products, per-unit output counts..., feed_dest, destinations...].
  const std::size_t num_units_size = static_cast<std::size_t>(parsed_num_units);
  const std::size_t feed_index = 3 + num_units_size;
  if (circuit_span.size() < feed_index + 1) {
    return false;
  }

  std::vector<int> parsed_output_counts(num_units_size);
  std::size_t total_outputs = 0;
  for (std::size_t unit_id = 0; unit_id < num_units_size; ++unit_id) {
    const int output_count = circuit_span[3 + unit_id];
    if (output_count != 2 && output_count != 3) {
      return false;
    }
    parsed_output_counts[unit_id] = output_count;
    total_outputs += static_cast<std::size_t>(output_count);
  }

  // Expected length is header + unit type list + feed destination + all unit destinations.
  const std::size_t expected_length = 3 + num_units_size + 1 + total_outputs;
  if (circuit_span.size() != expected_length) {
    return false;
  }

  const int parsed_feed_dest = circuit_span[feed_index];
  if (parsed_feed_dest < 0 || parsed_feed_dest >= parsed_num_units) {
    return false;
  }

  std::vector<CUnit> parsed_units(num_units_size, CUnit{});

  std::size_t destination_index = feed_index + 1;
  for (std::size_t unit_id = 0; unit_id < num_units_size; ++unit_id) {
    CUnit& unit = parsed_units[unit_id];
    unit.n_outputs = parsed_output_counts[unit_id];
    unit.unit_type = unit.n_outputs == 2 ? 0 : 1;
    unit.output.assign(static_cast<std::size_t>(unit.n_outputs), 0);

    for (int output_id = 0; output_id < unit.n_outputs; ++output_id) {
      unit.output[static_cast<std::size_t>(output_id)] =
          circuit_span[destination_index++];
    }
  }

  // Replace old state only after parsing succeeds, so repeated initialise calls are safe.
  num_inputs_ = parsed_num_inputs;
  num_units_ = parsed_num_units;
  num_products_ = parsed_num_products;
  feed_dest_ = parsed_feed_dest;
  output_counts_ = parsed_output_counts;
  units = parsed_units;

  return true;
}

bool Circuit::is_unit_id(int id) const noexcept {
  // Unit ids occupy [0, num_units); product ids start immediately afterwards.
  return id >= 0 && id < num_units_;
}

bool Circuit::is_product_id(int id) const noexcept {
  // Product ids occupy [num_units, num_units + num_products).
  return id >= num_units_ && id < num_units_ + num_products_;
}

int Circuit::product_index(int product_id) const noexcept {
  return product_id - num_units_;
}

std::vector<bool> Circuit::units_reachable_from_feed() const {
  std::vector<bool> seen(static_cast<std::size_t>(std::max(num_units_, 0)), false);
  if (!is_unit_id(feed_dest_)) {
    return seen;
  }

  // Recycle loops are allowed; seen prevents traversal from circling forever.
  std::vector<int> stack{feed_dest_};
  while (!stack.empty()) {
    const int unit_id = stack.back();
    stack.pop_back();

    if (!is_unit_id(unit_id)) {
      continue;
    }

    const std::size_t unit_index = static_cast<std::size_t>(unit_id);
    if (seen[unit_index]) {
      continue;
    }

    seen[unit_index] = true;
    for (const int destination : units[unit_index].output) {
      if (is_unit_id(destination)) {
        stack.push_back(destination);
      }
      // Product destinations are terminal streams, so traversal stops there.
    }
  }

  return seen;
}

bool Circuit::can_reach(int start, int target) const {
  if (!is_unit_id(start)) {
    return false;
  }

  // Reachability is a graph query only; self-recycle is checked by the rule layer.
  std::vector<bool> visited(static_cast<std::size_t>(std::max(num_units_, 0)), false);
  std::vector<int> stack{start};

  while (!stack.empty()) {
    const int unit_id = stack.back();
    stack.pop_back();

    if (!is_unit_id(unit_id)) {
      continue;
    }

    const std::size_t unit_index = static_cast<std::size_t>(unit_id);
    if (visited[unit_index]) {
      continue;
    }

    visited[unit_index] = true;
    for (const int destination : units[unit_index].output) {
      if (destination == target) {
        return true;
      }
      if (is_unit_id(destination)) {
        stack.push_back(destination);
      }
      // Products have no outgoing edges in this structural graph.
    }
  }

  return false;
}

std::vector<bool> Circuit::products_reachable_from_unit(int unit_id) const {
  std::vector<bool> product_seen(static_cast<std::size_t>(std::max(num_products_, 0)), false);
  if (!is_unit_id(unit_id)) {
    return product_seen;
  }

  // visited handles ordinary recycle loops without marking them invalid.
  std::vector<bool> visited(static_cast<std::size_t>(std::max(num_units_, 0)), false);
  std::vector<int> stack{unit_id};

  while (!stack.empty()) {
    const int current_unit_id = stack.back();
    stack.pop_back();

    if (!is_unit_id(current_unit_id)) {
      continue;
    }

    const std::size_t current_unit_index = static_cast<std::size_t>(current_unit_id);
    if (visited[current_unit_index]) {
      continue;
    }

    visited[current_unit_index] = true;
    for (const int destination : units[current_unit_index].output) {
      if (is_product_id(destination)) {
        product_seen[static_cast<std::size_t>(product_index(destination))] = true;
      } else if (is_unit_id(destination)) {
        stack.push_back(destination);
      }
      // Invalid destinations are ignored here; range validity belongs to rule checks.
    }
  }

  return product_seen;
}

int Circuit::count_reachable_products_from_unit(int unit_id) const {
  const std::vector<bool> reachable_products = products_reachable_from_unit(unit_id);
  return static_cast<int>(std::count(reachable_products.begin(), reachable_products.end(), true));
}

int Circuit::num_inputs() const noexcept {
  return num_inputs_;
}

int Circuit::num_units() const noexcept {
  return num_units_;
}

int Circuit::num_products() const noexcept {
  return num_products_;
}

int Circuit::feed_dest() const noexcept {
  return feed_dest_;
}

const std::vector<int>& Circuit::output_destinations(int unit_id) const {
  if (!is_unit_id(unit_id)) {
    return empty_outputs_;
  }

  return units[static_cast<std::size_t>(unit_id)].output;
}

bool Circuit::check_validity(int vector_size, int *circuit_vector) {
  if (vector_size < 0 || circuit_vector == nullptr) {
    return false;
  }

  Circuit circuit;
  return circuit.initialise(
      std::span<const int>(circuit_vector, static_cast<std::size_t>(vector_size)));
}

bool Circuit::check_validity(int vector_size,
                             int *circuit_vector,
                             int unit_parameters_size,
                             double *unit_parameters) {
  (void)unit_parameters_size;
  (void)unit_parameters;
  return check_validity(vector_size, circuit_vector);
}

void Circuit::mark_units(int unit_num) {

  if (!is_unit_id(unit_num)) return;

  if (units[unit_num].mark) return;

  units[unit_num].mark = true;

  //If we have seen this unit already exit
  //Mark that we have now seen the unit

  for (int i=0; i<units[unit_num].n_outputs; i++) {
    //If output_unit_index does not point at a circuit outlet recursively call the function
    const int destination = units[unit_num].output[static_cast<std::size_t>(i)];
    if (is_unit_id(destination)) {
      mark_units(destination);
    } else {
      // ...Potentially do something to indicate that you have seen an exit
    }
  }

}


