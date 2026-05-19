// Economics.h
//
// Once the solver has flows in the two product streams, the GA needs one
// fitness number in GBP/s. This header does that conversion.
//
// All £/kg numbers sit in EconomicConstants (appendix defaults). Change them
// there when we sweep prices in the sensitivity study.
//
// Unit running cost is pluggable via OpCostFn. Today that is fixed_op_cost;
// a volume power-law can be added later as another function.

#pragma once

#include <span>

namespace cuprite {

// Mass flows in kg/s for one stream.
struct Stream {
    double pal = 0.0;
    double gor = 0.0;
    double waste = 0.0;
};

// Appendix price table, plus £/s to run each unit type.
struct EconomicConstants {
    // Palusznium product stream (£/kg of each species in that stream)
    double pal_in_pal_stream = +120.0;
    double gor_in_pal_stream = -20.0;
    double waste_in_pal_stream = -500.0;
    // Gormanium product stream
    double pal_in_gor_stream = -5.0;
    double gor_in_gor_stream = +100.0;
    double waste_in_gor_stream = -500.0;
    // Running cost per unit per second
    double op_cost_type_A_per_s = 10.0;
    double op_cost_type_B_per_s = 12.0;
};

extern const EconomicConstants default_economics;

// How many A/B units are in the circuit. volumes_* are for a future cost
// model; fixed_op_cost ignores them.
struct CircuitDescriptor {
    int n_A = 0;
    int n_B = 0;
    std::span<const double> volumes_A{};
    std::span<const double> volumes_B{};
};

// Function pointer so we can swap running-cost models without a vtable.
using OpCostFn = double (*)(const CircuitDescriptor&, const EconomicConstants&);

double fixed_op_cost(const CircuitDescriptor& circuit, const EconomicConstants& econ);

// Net GBP/s for a converged run. Only the two product streams matter.
double economic_value(const Stream& pal_product, const Stream& gor_product,
                      const CircuitDescriptor& circuit, OpCostFn op_cost = &fixed_op_cost,
                      const EconomicConstants& econ = default_economics);

// Solver did not converge: waste in feed × waste penalty (-40000 at base feed).
double worst_case_value(double waste_feed_kg_per_s,
                        const EconomicConstants& econ = default_economics);

}  // namespace cuprite
