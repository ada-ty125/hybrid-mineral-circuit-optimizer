#include "CUnit.h"
#include <cmath>

CUnit::CUnit() {
    k[0] = 0.006;   // example Palusznium
    k[1] = 0.0005;  // example Gormanium
    k[2] = 0.0005;  // example Waste
}

double CUnit::total_feed() const{
  double total_feed = 0.0;
  for (int i = 0; i < N_COMPONENTS; i++) {
        total_feed += feed[i];
    }

}

double CUnit::calculate_residence_time() const {
    double total_mass_flow = total_feed();

    if (total_mass_flow < 1e-10) {
        total_mass_flow = 1e-10;
    }
    double total_vol_flow = total_mass_flow / rho;
    return (phi * volume) / total_vol_flow;
}

double CUnit::calculate_recovery(int component) const {
    double tau = calculate_residence_time();

    return (k[component] * tau) / (1.0 + k[component] * tau);
}
void CUnit::calculate_outputs() {
  concentrate.resize(n_outputs - 1);
  double total_recovery[N_COMPONENTS] = {0.0, 0.0, 0.0};
  for (int out = 0; out < n_outputs - 1; out++) {
    for (int comp = 0; comp < N_COMPONENTS; comp++) {
      double R = calculate_recovery(comp);
      concentrate[out][comp] = feed[comp] * R;
      total_recovery[comp] += R;
    }
  }
  for (int comp = 0; comp < N_COMPONENTS; comp++) {
    tails[comp] = feed[comp] * (1.0 - total_recovery[comp]);
  }
}

