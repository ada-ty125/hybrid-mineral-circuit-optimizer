// bench_economics.cpp
//
// Times economic_value and writes a CSV under tests/bin/benchmarks/ so we
// can compare later op-cost models against this baseline.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "Economics.h"

#ifndef CUPRITE_GIT_SHA
#define CUPRITE_GIT_SHA "unknown"
#endif

#ifndef CUPRITE_BENCH_OUTPUT_DIR
#define CUPRITE_BENCH_OUTPUT_DIR "."
#endif

using cuprite::CircuitDescriptor;
using cuprite::default_economics;
using cuprite::economic_value;
using cuprite::fixed_op_cost;
using cuprite::Stream;
using cuprite::worst_case_value;

namespace {

// ---------- reference CSV loader & accuracy ----------------------------------

struct RefCase {
    Stream pal_product;
    Stream gor_product;
    CircuitDescriptor circuit;
    double expected = 0.0;
};

std::vector<RefCase> load_reference(const std::string& path) {
    std::vector<RefCase> cases;
    std::ifstream f(path);
    if (!f.is_open()) {
        std::fprintf(stderr,
                     "bench_economics: WARN: reference CSV not found at %s; "
                     "accuracy column will be NaN\n",
                     path.c_str());
        return cases;
    }
    std::string line;
    std::getline(f, line);  // header
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string tok;
        std::vector<std::string> cols;
        while (std::getline(ss, tok, ',')) cols.push_back(tok);
        if (cols.size() < 10) continue;
        RefCase c;
        c.pal_product.pal = std::stod(cols[1]);
        c.pal_product.gor = std::stod(cols[2]);
        c.pal_product.waste = std::stod(cols[3]);
        c.gor_product.pal = std::stod(cols[4]);
        c.gor_product.gor = std::stod(cols[5]);
        c.gor_product.waste = std::stod(cols[6]);
        c.circuit.n_A = std::stoi(cols[7]);
        c.circuit.n_B = std::stoi(cols[8]);
        c.expected = std::stod(cols[9]);
        cases.push_back(c);
    }
    return cases;
}

double max_abs_err_against_reference(const std::vector<RefCase>& ref) {
    if (ref.empty()) return std::nan("");
    double worst = 0.0;
    for (const auto& c : ref) {
        const double got = economic_value(c.pal_product, c.gor_product, c.circuit);
        worst = std::max(worst, std::fabs(got - c.expected));
    }
    return worst;
}

// ---------- per-benchmark metadata (op-cost model name) ---------------------

std::map<std::string, std::string> g_op_cost_model;

void register_op_cost_model(const char* bench_name, const char* model_name) {
    g_op_cost_model[bench_name] = model_name;
}

// ---------- benchmarks -------------------------------------------------------

const Stream kEmpty;
const Stream kRealisticPal{2.0, 0.1, 0.05};
const Stream kRealisticGor{0.05, 3.0, 0.04};
const CircuitDescriptor kCircuit_7A_3B{7, 3, {}, {}};

void BM_EconomicValue_FixedOpCost_Empty(benchmark::State& state) {
    for (auto _ : state) {
        double v =
            economic_value(kEmpty, kEmpty, kCircuit_7A_3B, &fixed_op_cost, default_economics);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_EconomicValue_FixedOpCost_Empty);

void BM_EconomicValue_FixedOpCost_Realistic(benchmark::State& state) {
    for (auto _ : state) {
        double v = economic_value(kRealisticPal, kRealisticGor, kCircuit_7A_3B, &fixed_op_cost,
                                  default_economics);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_EconomicValue_FixedOpCost_Realistic);

void BM_FixedOpCost_Only(benchmark::State& state) {
    for (auto _ : state) {
        double v = fixed_op_cost(kCircuit_7A_3B, default_economics);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_FixedOpCost_Only);

void BM_WorstCase(benchmark::State& state) {
    for (auto _ : state) {
        double v = worst_case_value(80.0, default_economics);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_WorstCase);

void BM_Batch1000(benchmark::State& state) {
    for (auto _ : state) {
        double acc = 0.0;
        for (int i = 0; i < 1000; ++i) {
            acc += economic_value(kRealisticPal, kRealisticGor, kCircuit_7A_3B, &fixed_op_cost,
                                  default_economics);
        }
        benchmark::DoNotOptimize(acc);
    }
}
BENCHMARK(BM_Batch1000)->Threads(1);

// ---------- CSV reporter -----------------------------------------------------

struct RowBuffer {
    std::vector<benchmark::BenchmarkReporter::Run> runs;
};

class EconomicsCsvReporter : public benchmark::ConsoleReporter {
  public:
    void ReportRuns(const std::vector<Run>& report) override {
        // Print to console as usual, but keep a copy for the CSV at the end.
        benchmark::ConsoleReporter::ReportRuns(report);
        for (const auto& r : report) buf_.runs.push_back(r);
    }

    const RowBuffer& buffer() const { return buf_; }

  private:
    RowBuffer buf_;
};

std::string iso8601_now() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

std::string iso8601_compact_for_filename() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%dT%H%M%SZ", &tm);
    return buf;
}

void write_csv(const RowBuffer& buf, double max_err) {
    const std::string dir = CUPRITE_BENCH_OUTPUT_DIR;
    const std::string path = dir + "/economics_" + iso8601_compact_for_filename() + ".csv";
    std::ofstream out(path);
    if (!out.is_open()) {
        std::fprintf(stderr, "bench_economics: WARN: cannot write CSV to %s\n", path.c_str());
        return;
    }
    out << "timestamp,git_sha,benchmark_name,op_cost_model,iterations,"
           "real_time_ns,cpu_time_ns,max_abs_err,notes\n";
    const std::string ts = iso8601_now();
    out << std::setprecision(6);
    for (const auto& r : buf.runs) {
        const std::string name = r.benchmark_name();
        auto it = g_op_cost_model.find(name);
        const std::string model = it != g_op_cost_model.end() ? it->second : "fixed_op_cost";
        // Benchmark gives total seconds; we want ns per iteration in the CSV.
        const double iters = static_cast<double>(r.iterations);
        const double real_ns = iters > 0 ? (r.real_accumulated_time / iters) * 1e9 : 0.0;
        const double cpu_ns = iters > 0 ? (r.cpu_accumulated_time / iters) * 1e9 : 0.0;
        out << ts << "," << CUPRITE_GIT_SHA << "," << name << "," << model << "," << r.iterations
            << "," << real_ns << "," << cpu_ns << "," << std::setprecision(17) << max_err
            << ",baseline\n"
            << std::setprecision(6);
    }
    std::printf("bench_economics: wrote %s\n", path.c_str());
}

}  // namespace

int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

    // Tag each benchmark with the op-cost model name for the CSV.
    register_op_cost_model("BM_EconomicValue_FixedOpCost_Empty", "fixed_op_cost");
    register_op_cost_model("BM_EconomicValue_FixedOpCost_Realistic", "fixed_op_cost");
    register_op_cost_model("BM_FixedOpCost_Only", "fixed_op_cost");
    register_op_cost_model("BM_WorstCase", "n/a");
    register_op_cost_model("BM_Batch1000", "fixed_op_cost");

    // Check all reference rows once; same number goes in every CSV row.
    const auto ref = load_reference("data/economics_reference.csv");
    const double max_err = max_abs_err_against_reference(ref);
    std::printf("bench_economics: max_abs_err vs reference = %.17g\n", max_err);

    EconomicsCsvReporter rep;
    benchmark::RunSpecifiedBenchmarks(&rep);
    benchmark::Shutdown();
    write_csv(rep.buffer(), max_err);
    return 0;
}
