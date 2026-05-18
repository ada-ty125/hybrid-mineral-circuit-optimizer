/** Header for the unit class
 * 
 * 
 */

#pragma once

#include <vector>
#include <array>


constexpr int N_COMPONENTS = 3

class CUnit {

 public:

  int n_outputs; // The number of outputs from this unit

  //A Boolean that is changed to true if the unit has been seen 
  bool mark;

  // Output from this unit where [0] = Palusznium, [1] = Gormanium, [2] = Waste
  std::vector<int> output;

  // Current feed entering this unit
  // [0] = Palusznium
  // [1] = Gormanium
  // [2] = Waste
  double feed[N_COMPONENTS] = {0.0, 0.0, 0.0};

  // Feed from previous iteration
  double old_feed[N_COMPONENTS] = {0.0, 0.0, 0.0};

  // Concentrate output streams
  // concentrate[i][j]
  // i = output stream number
  // j = component
  std::vector<std::array<double, N_COMPONENTS>> concentrate;

  // Tailings stream
  double tails[N_COMPONENTS] = {0.0, 0.0, 0.0};

  // Used for validity checking
  bool mark = false;

  // Constants for calculations
  double volume = 10.0;
  double phi = 0.1;
  double rho = 3000.0;

  // Recovery constants
  // k[0] = Palusznium
  // k[1] = Gormanium
  // k[2] = Waste
  double k[N_COMPONENTS] = {0.0, 0.0, 0.0};

  // Simulation functions
  double calculate_residence_time() const;

  double calculate_recovery(int component) const;

  void calculate_outputs();

  void clear_feeds();
};

