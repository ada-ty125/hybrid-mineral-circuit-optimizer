#include "CUnit.h"

#include <cmath>

double CUnit::total_feed() const {
  double total_feed = 0.0;
  for (int i = 0; i < N_COMPONENTS; i++) {
    total_feed += feed[i];
  }
  return total_feed;
}

double CUnit::calculate_residence_time() const {
  double total_mass_flow = total_feed();

  if (total_mass_flow < 1e-10) {
    total_mass_flow = 1e-10;
  }
  double total_vol_flow = total_mass_flow / rho;
  return (phi * volume) / total_vol_flow;
}

double CUnit::calculate_recovery(int st_idx, int component) const {
  double tau = calculate_residence_time();
  double summation = 0.0;
  int num_c_stream = n_outputs - 1;
  for (int m = 0; m < num_c_stream; m++) {
    summation += k_matrix[m][component] * tau;
  }
  double num = k_matrix[st_idx][component] * tau;

  return num / (1 + summation);
}
void CUnit::calculate_outputs() {
  int num_c_streams = n_outputs - 1;
  concentrate.resize(num_c_streams);
  double total_recovery[N_COMPONENTS] = {0.0, 0.0, 0.0};
  for (int out = 0; out < num_c_streams; out++) {
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
      double R = calculate_recovery(out, comp);
      concentrate[out][comp] = feed[comp] * R;
      total_recovery[comp] += R;
    }
  }
  for (int comp = 0; comp < N_COMPONENTS; comp++) {
    tails[comp] = feed[comp] * (1.0 - total_recovery[comp]);
  }
}
