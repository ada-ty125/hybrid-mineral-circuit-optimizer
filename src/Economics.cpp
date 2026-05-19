// Economics.cpp
//
// Pure maths, no I/O. The GA will call these functions a lot each generation.

#include "Economics.h"

namespace cuprite {

const EconomicConstants default_economics{};

double fixed_op_cost(const CircuitDescriptor& circuit, const EconomicConstants& econ) {
    return econ.op_cost_type_A_per_s * circuit.n_A + econ.op_cost_type_B_per_s * circuit.n_B;
}

double economic_value(const Stream& pal_product, const Stream& gor_product,
                      const CircuitDescriptor& circuit, OpCostFn op_cost,
                      const EconomicConstants& econ) {
    // Both concentrates only. Tailings are not priced (brief says no cost).
    const double revenue =
        econ.pal_in_pal_stream * pal_product.pal + econ.gor_in_pal_stream * pal_product.gor +
        econ.waste_in_pal_stream * pal_product.waste + econ.pal_in_gor_stream * gor_product.pal +
        econ.gor_in_gor_stream * gor_product.gor + econ.waste_in_gor_stream * gor_product.waste;

    return revenue - op_cost(circuit, econ);
}

double worst_case_value(double waste_feed_kg_per_s, const EconomicConstants& econ) {
    // Brief fallback when the mass balance does not converge.
    return waste_feed_kg_per_s * econ.waste_in_pal_stream;
}

}  // namespace cuprite
