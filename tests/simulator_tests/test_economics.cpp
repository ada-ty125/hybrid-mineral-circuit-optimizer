// test_economics.cpp
//
// Expected values worked out by hand from the appendix. Exits 1 on failure.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "Economics.h"

using cuprite::CircuitDescriptor;
using cuprite::default_economics;
using cuprite::economic_value;
using cuprite::fixed_op_cost;
using cuprite::Stream;
using cuprite::worst_case_value;

namespace {

constexpr double kTol = 1e-9;

int failures = 0;

void check_close(const char* name, double got, double expected) {
    const double err = std::fabs(got - expected);
    if (err <= kTol) {
        std::printf("PASS  %-50s got=%g\n", name, got);
    } else {
        std::printf("FAIL  %-50s got=%g expected=%g (|err|=%g)\n", name, got, expected, err);
        ++failures;
    }
}

void check_bits_equal(const char* name, double a, double b) {
    if (std::memcmp(&a, &b, sizeof(double)) == 0) {
        std::printf("PASS  %-50s both=%g\n", name, a);
    } else {
        std::printf("FAIL  %-50s a=%g b=%g (bits differ)\n", name, a, b);
        ++failures;
    }
}

}  // namespace

int main() {
    std::printf("test_economics: running hand-computed accuracy cases\n");

    const Stream zero;
    CircuitDescriptor circuit_1A{1, 0, {}, {}};
    CircuitDescriptor circuit_7A_3B{7, 3, {}, {}};
    CircuitDescriptor circuit_0{0, 0, {}, {}};

    // 1) empty streams + 1 type A unit -> -10 GBP/s
    check_close("1. empty + 1A -> op cost only", economic_value(zero, zero, circuit_1A), -10.0);

    // 2) empty streams + 7A + 3B -> -10*7 - 12*3 = -106 GBP/s
    check_close("2. empty + 7A+3B -> op cost only", economic_value(zero, zero, circuit_7A_3B),
                -106.0);

    // 3) pal_product = {1,0,0}, no units -> +120 GBP/s
    {
        Stream pal{1.0, 0.0, 0.0};
        check_close("3. pal product = 1 kg/s palusznium", economic_value(pal, zero, circuit_0),
                    120.0);
    }

    // 4) pal_product = {0,1,0}, no units -> -20 GBP/s
    {
        Stream pal{0.0, 1.0, 0.0};
        check_close("4. pal product = 1 kg/s gormanium (penalty)",
                    economic_value(pal, zero, circuit_0), -20.0);
    }

    // 5) pal_product = {0,0,1}, no units -> -500 GBP/s
    {
        Stream pal{0.0, 0.0, 1.0};
        check_close("5. pal product = 1 kg/s waste (penalty)", economic_value(pal, zero, circuit_0),
                    -500.0);
    }

    // 6) gor_product = {1,0,0}, no units -> -5 GBP/s
    {
        Stream gor{1.0, 0.0, 0.0};
        check_close("6. gor product = 1 kg/s palusznium (penalty)",
                    economic_value(zero, gor, circuit_0), -5.0);
    }

    // 7) gor_product = {0,1,0}, no units -> +100 GBP/s
    {
        Stream gor{0.0, 1.0, 0.0};
        check_close("7. gor product = 1 kg/s gormanium", economic_value(zero, gor, circuit_0),
                    100.0);
    }

    // 8) worst_case_value(80) -> -40000 GBP/s (base feed waste rate)
    check_close("8. worst-case fitness (waste = 80 kg/s)", worst_case_value(80.0), -40000.0);

    // 9) Same inputs twice must give the same bits (no hidden state).
    {
        Stream pal{1.0, 2.0, 3.0};
        Stream gor{4.0, 5.0, 6.0};
        const double a = economic_value(pal, gor, circuit_7A_3B);
        const double b = economic_value(pal, gor, circuit_7A_3B);
        check_bits_equal("9. determinism: repeat call returns identical bits", a, b);
    }

    // 10) fixed_op_cost on its own, to lock in the baseline strategy.
    check_close("10. fixed_op_cost(7A+3B) directly",
                fixed_op_cost(circuit_7A_3B, default_economics), 106.0);

    // 11) Realistic-ish mixed case, computed by hand:
    //     pal stream: pal=2, gor=0.1, waste=0.05
    //     gor stream: pal=0.05, gor=3, waste=0.04
    //     units: 7A + 3B
    //     revenue = 120*2 + -20*0.1 + -500*0.05
    //             + -5*0.05 + 100*3 + -500*0.04
    //             = 240 - 2 - 25 - 0.25 + 300 - 20
    //             = 492.75
    //     op cost = 10*7 + 12*3 = 106
    //     net     = 492.75 - 106 = 386.75
    {
        Stream pal{2.0, 0.1, 0.05};
        Stream gor{0.05, 3.0, 0.04};
        check_close("11. realistic mixed case", economic_value(pal, gor, circuit_7A_3B), 386.75);
    }

    if (failures == 0) {
        std::printf("test_economics: ALL PASS\n");
        return 0;
    }
    std::printf("test_economics: %d FAILURES\n", failures);
    return 1;
}
